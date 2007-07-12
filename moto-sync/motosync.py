"""
 opensync plugin for syncing to a Motorola mobile phone
 HIGHLY EXPERIMENTAL, USE AT YOUR OWN RISK!
"""

# Copyright (C) 2006-2007  Andrew Baumann <andrewb@cse.unsw.edu.au>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of version 2 of the GNU General Public License
#  as published by the Free Software Foundation.

__revision__ = "$Id$"

import os, types, md5, time, calendar, re
import xml.dom.minidom
from datetime import date, datetime, timedelta, time as datetime_time
import dateutil.parser, dateutil.rrule as rrule, dateutil.tz
import opensync

# optionally use the 'tty' module that is only present on Unix
try:
    import tty
    USE_TTY_MODULE = True
except ImportError:
    USE_TTY_MODULE = False

# optionally use the 'bluetooth' module from pybluez
try:
    import bluetooth
    USE_BLUETOOTH_MODULE = True
except ImportError:
    USE_BLUETOOTH_MODULE = False


# Regular expression for a Bluetooth MAC address
BT_MAC_RE = re.compile(r'^([0-9a-f]{2}:){5}[0-9a-f]{2}$', re.IGNORECASE)

# name of the SDP service to use with Bluetooth
BT_SERVICE_NAME = 'Dial-up networking Gateway'
# name of the channel to try if we don't find the above service
BT_DEFAULT_CHANNEL = 1

# object types supported by this plugin
SUPPORTED_OBJTYPES = ['event', 'contact']

# my phone doesn't like it if you read more than this many entries at a time
ENTRIES_PER_READ = 15

# time and date formats
PHONE_TIME = '%H:%M'
PHONE_DATE = '%m-%d-%Y' # yuck!
VCAL_DATETIME = '%Y%m%dT%H%M%SZ'
VCAL_DATE = '%Y%m%d'

# days of the week in vcal parlance
VCAL_DAYS = ['MO', 'TU', 'WE', 'TH', 'FR', 'SA', 'SU']

# days of week as constants in the dateutil.rrule module
RRULE_DAYS = [rrule.MO, rrule.TU, rrule.WE, rrule.TH, rrule.FR, rrule.SA, rrule.SU]

# repeat types in the calendar
# these have a different meaning in the simple and extended calendars (why???!)
MOTO_REPEAT_NONE = 0
MOTO_REPEAT_DAILY = 1
MOTO_REPEAT_WEEKLY = 2
MOTO_REPEAT_SIMPLE_MONTHLY_DATE = 3
MOTO_REPEAT_SIMPLE_MONTHLY_DAY = 4
MOTO_REPEAT_SIMPLE_YEARLY_DATE = 5
MOTO_REPEAT_EXTENDED_MONTHLY_DAY = 3
MOTO_REPEAT_EXTENDED_MONTHLY_DATE = 4
MOTO_REPEAT_EXTENDED_YEARLY_DAY = 5
MOTO_REPEAT_EXTENDED_YEARLY_DATE = 6

# features we require; these refer to the bits returned by the AT+MAID? command
# FIXME: my phone returns a lot more bits than I have documentation for, it's
#        likely that we actually depend on more than just these features
REQUIRED_FEATURES = [
    (0, 'phone book'),
    (1, 'date book'),
    (4, 'email addresses'),
    (7, 'shared dynamic memory phone/date book'),
    (9, 'distinctive alert in phone book')
]

# mapping from phone's contact type to XML number types
MOTO_PHONE_CONTACT_TYPES = {
    2: 'Voice',
    3: 'Cellular',
    4: 'Fax',
    5: 'Pager',
    # 6: email
    # 7: mailing list
    8: 'Cellular', # shows up as "Mobile 2", seen on a V3c
    # 9: ?
    # 10: ?
    11: 'Voice' # shows up as "other", seen on a RAZR V3x
}

# as above, but to TelephoneLocation
MOTO_PHONE_CONTACT_LOCATIONS = {
    0: 'Work',
    1: 'Home'
}

# reverse of the above (almost): mapping from vcard to phone's contact type
VCARD_CONTACT_TYPES = {
    'work':     0,
    'home':     1,
    'voice':    2,
    'cellular': 3,
    'car':      3,
    'message':  5,
    'fax':      4,
    'pager':    5,
}

# (dodgy) mapping from address types to motorola contact types
# can also have dom/intl/postal/parcel... these don't really make sense here
VCARD_ADDRESS_TYPES = {
    'work':  0,
    'home':  1,
}

# reverse of the above
MOTO_ADDRESS_TYPES = {
    0: 'Work',
    1: 'Home',
}

# mapping from phone's event type value to category strings
MOTO_EVENT_TYPES = {
    0: None,
    1: "Private",
    2: "Discussion",
    3: "Meeting",
    4: "Birthday",
    5: "Anniversary",
    6: "Telephone Call",
    7: "Vacation",
    8: "Holiday",
    9: "Entertainment",
    10: "Breakfast",
    11: "Lunch",
    12: "Dinner",
    13: "Education",
    14: "Travel",
    15: "Party",
}

# reverse of above
REVERSE_MOTO_EVENT_TYPES = {}
for (k, v) in MOTO_EVENT_TYPES.items():
    if v:
        REVERSE_MOTO_EVENT_TYPES[v.lower()] = k

MOTO_EVENT_TYPE_NONE = 0

# if not one of the above, we use:
MOTO_CONTACT_DEFAULT = 2 # "Main"

# special 'contact types' for email or mailing list entries
MOTO_CONTACT_EMAIL = 6
MOTO_CONTACT_MAILINGLIST = 7

# default category number
MOTO_CATEGORY_DEFAULT = 1

# each entry in the phonebook has one of these magic numbers
MOTO_NUMTYPE_LOCAL = 129
MOTO_NUMTYPE_INTL = 145
MOTO_NUMTYPE_UNKNOWN = 128

# various fields use 255 as an invalid value
MOTO_INVALID = 255
MOTO_INVALID_DATE = '00-00-2000'
MOTO_INVALID_TIME = '00:00'

# number of quick-dial entries in the phonebook; we avoid allocating these
MOTO_QUICKDIAL_ENTRIES = 10

# logical order of the fields in structured XML data
XML_NAME_PARTS = 'Prefix FirstName Additional LastName Suffix'.split()
XML_ADDRESS_PARTS = ('Street ExtendedAddress Locality Region PostalCode Country').split()

# legal characters in telephone numbers
TEL_NUM_DIGITS = set('+0123456789')

# how far into the future to process exceptions for recurring events
RRULE_FUTURE = timedelta(365) # 1 year


def getElementsByTagNames(parent, tagnames, ret):
    """Like DOM's getElementsByTagName, but allow multiple tag names."""
    for node in parent.childNodes:
        if (node.nodeType == xml.dom.minidom.Node.ELEMENT_NODE and node.tagName in tagnames):
            ret.append(node)
        getElementsByTagNames(node, tagnames, ret)
    return ret

def getXMLField(doc, tagname, subtag=None):
    """Returns text in a given XML tag, or None if not set.

    The XML structure that it looks for is:
        <tagname><subtag>text here</subtag></tagname>.
    It always returns the data in the first subtag of the first tag,
    repeated tags are ignored.
    """
    elts = doc.getElementsByTagName(tagname)
    if elts == []:
        return None
    elt = elts[0]
    if subtag:
        children = elt.getElementsByTagName(subtag)
        if children == []:
            return None
        elt = children[0]
    return getXMLText(elt)

def getXMLText(el):
    """Returns all text within a given XML node."""
    return ''.join([n.data for n in el.childNodes if n.nodeType == n.TEXT_NODE])

def appendXMLTag(doc, parent, tag, text):
    """Append a child tag with supplied text content to the parent node."""
    if text and text != '':
        e = doc.createElement(tag)
        e.appendChild(doc.createTextNode(text.encode('utf8')))
        parent.appendChild(e)

def insertXMLNode(parent, newnode):
    """Insert a new node into a parent's list of children, maintaining sort order."""
    for node in parent.childNodes:
        if newnode.nodeName < node.nodeName:
            parent.insertBefore(newnode, node)
            return
    parent.appendChild(newnode)

def insertXMLTag(doc, parent, tag, text):
    """Append a child tag with supplied text content to the parent node."""
    if text and text != '':
        e = doc.createElement(tag)
        e.appendChild(doc.createTextNode(text.encode('utf8')))
        insertXMLNode(parent, e)

def parse_moto_time(datestr, timestr=None):
    """convert phone's date and time string into a datetime object"""
    if timestr:
        t = time.strptime(datestr+' '+timestr, PHONE_DATE+' '+PHONE_TIME)
        # assume that phone time is in our local timezone, convert to UTC
        return datetime.fromtimestamp(time.mktime(t), dateutil.tz.tzutc())
    else:
        t = time.strptime(datestr, PHONE_DATE)
        return date.fromtimestamp(time.mktime(t))

# this duration-parsing code lifted from the python-icalendar library
# http://codespeak.net/icalendar/
DATE_PART = r'(\d+)D'
TIME_PART = r'T(?:(\d+)H)?(?:(\d+)M)?(?:(\d+)S)?'
DATETIME_PART = '(?:%s)?(?:%s)?' % (DATE_PART, TIME_PART)
WEEKS_PART = r'(\d+)W'
DURATION_REGEX = re.compile(r'([-+]?)P(?:%s|%s)$' % (WEEKS_PART, DATETIME_PART))

def parse_ical_duration(duratstr):
    """Parse an ical duration string, returning a timedelta object."""
    try:
        match = DURATION_REGEX.match(duratstr)
        sign, weeks, days, hours, minutes, seconds = match.groups()
    except (AttributeError, ValueError):
        raise ValueError('Invalid iCalendar duration: %s' % duratstr)
    if weeks:
        value = timedelta(weeks=int(weeks))
    else:
        value = timedelta(days=int(days or 0), hours=int(hours or 0),
                          minutes=int(minutes or 0), seconds=int(seconds or 0))
    if sign == '-':
        value = -value
    return value

def parse_ical_time(timestr):
    """Parse an ical time string, return either a datetime or date object."""
    d = dateutil.parser.parse(timestr)
    if len(timestr) == 8: # just a date
        assert(d.hour == d.minute == d.second == 0)
        return d.date()
    else:
        return d

def format_time(dt, format):
    """Convert datetime object into local time and format as a string"""
    if isinstance(dt, datetime) and dt.utcoffset() == timedelta(0):
        # FIXME: assumes local timezone
        t = time.localtime(calendar.timegm(dt.utctimetuple()))
        return time.strftime(format, t)
    else:
        # already in localtime
        return dt.strftime(format)

def xml_rrule_to_dateutil(xmlnode, eventdt):
    """Convert an rrule from OpenSync XML format to a dateutil.rrule object."""

    # convert frequency string
    freqstr = getXMLField(xmlnode, 'Frequency').lower()
    if freqstr == 'daily':
        freq = rrule.DAILY
    elif freqstr == 'weekly':
        freq = rrule.WEEKLY
    elif freqstr == 'monthly':
        freq = rrule.MONTHLY
    elif freqstr == 'yearly':
        freq = rrule.YEARLY
    else:
        assert(False, "invalid frequency %s" % freqstr)

    until = getXMLField(xmlnode, 'Until')
    if until:
        until = parse_ical_time(until)

    count = getXMLField(xmlnode, 'Count')
    if count:
        count = int(count)

    interval = getXMLField(xmlnode, 'Interval')
    if interval:
        interval = int(interval)

    def getFieldAsList(name, convfunc=int):
        """Convenience function, get a list of comma-separated values from XML."""
        val = getXMLField(xmlnode, name)
        if val:
            return map(convfunc, val.split(','))
        else:
            return None

    bymonth = getFieldAsList('ByMonth')
    byyearday = getFieldAsList('ByYearDay')
    bymonthday = getFieldAsList('ByMonthDay')
    byday = getFieldAsList('ByDay', lambda s: s.upper())

    # convert byday strings to the constants needed by rrule
    byweekday = []
    for bydaystr in byday:
        day = RRULE_DAYS[VCAL_DAYS.index(bydaystr[-2:])]
        nth = bydaystr[:-2]
        if nth:
            byweekday.append(day(int(nth)))
        else:
            byweekday.append(day)

    # done!
    return rrule.rrule(freq, dtstart=eventdt, interval=interval, count=count,
                       until=until, bymonth=bymonth, bymonthday=bymonthday,
                       byweekday=byweekday, byyearday=byyearday)

def moto_repeat_day_to_weekdays(repeat_day,  eventday):
    """For a weekly repeat, return the list of days the event repeats on.

    repeat_day is field 19 (repeat on day) in the extended event format
    eventday is the 0-based weekday on which the event occurs

    This is based on the following description:

    The weekday of the event (the one that gets repeated of course) gets the
    value 2^6 = 64, the following weekday gets the value 2^5, the after that one
    gets 2^4, etc.
    Field 19 contains the sum of the binary encoded weekdays.

    For example: lets assume we have an event on Thursday, 04-12-2007
    As the starting weekday is the day of the first event (Thursday), this day
    gets encoded with 2^6, so the encoding for the whole week is as follows:
    thursday 2^6 (1000000), friday 2^5 (0100000), saturday 2^4 (0010000),
    sunday 2^3 (0001000), monday 2^2 (0000100), tuesday 2^1 (0000010),
    wednesday 2^0 (0000001)
    lets say we want to repeat that event every thursday and monday on a weekly basis.
    Field 19 would then contain 2^6 + 2^2 = 68 or in binary form 1000100
    """

    if repeat_day == 0:
        return []
    else:
        eventday = (eventday - 1) % 7
        rest = moto_repeat_day_to_weekdays(repeat_day / 2, eventday)
        if repeat_day % 2:
            return rest + [eventday]
        else:
            return rest

def moto_weekdays_to_repeat_day(weekdays, eventday):
    """Returns the repeat_day value given a set of days on which a weekly event repeats."""

    return sum([2 ** (6 - (day - eventday) % 7) for day in weekdays])

def moto_repeat_day_to_monthday(repeat_day):
    """For a monthly or yearly by day repeat, return the day the event repeats on.

    repeat_day is field 19 (repeat on day) in the extended event format
    returns a tuple (nth, day)
    nth is the Nth time the day occurs in the month
    day is the weekday number (0=monday)

    This is based on the following description:

    The weekdays get the following basis encodings :
    sunday = 0
    monday = 8
    tuesday = 16
    wednesday = 24
    thursday = 32
    friday = 40
    saturday = 48

    The above values are NEVER the value of field 19. Instead the field contains
    a value computed as follows:
    WEEKDAYCODE + 1 = first WEEKDAY of month
    WEEKDAYCODE + 2 = second WEEKDAY of month
    WEEKDAYCODE + 3 = third WEEKDAY of month
    WEEKDAYCODE + 4 = fourth WEEKDAY of month
    WEEKDAYCODE + 5 = last WEEKDAY of month
    """

    daynum = (repeat_day / 8 - 1) % 7
    nth = repeat_day % 8
    if nth > 4:
        nth = 4 - nth
    return (nth, daynum)

# reverse of the above function
def moto_monthday_to_repeat_day(nth, daynum):
    """For a monthly or yearly by day repeat, returns the repeat_day value given
    the Nth time a day occurs in a montyh, and the day of the week number.
    """
    if nth < 0:
        nth = 4 - nth
    return ((daynum + 1) % 7) * 8 + nth

def xml_rrule_to_moto(rulenodes, exdates, exrules, eventdt, extended_format):
    """Process XML recursion rules, converting them to the closest-possible
    matching recursion specifier supported by the phone.

    Arguments:
      rulenodes: list of RecurrenceRule nodes
      exdates:   list of exception dates
      exrules:   list of exception rules
      eventdt:   event's date (and time if set)
      extended_format: bool iff the extended calendar format is used

    Returns a dict containing the following fields:
      repeat_type:  Motorola repeat type (always present)
      repeat_every: repeat every Nth time (valid for the extended format only)
      repeat_day:   bitfield showing which days/weeks to repeat on (extended)
      repeat_end:   end date of the repeat (extended)
      exceptions:   list of exceptions (always present)

    The general approach we take is: if the recursion can be represented
    by the phone's data structure, we convert it, otherwise we ignore the
    rule completely (to avoid the event showing up at incorrect times) by
    raising an UnsupportedDataError exception.
    """

    # default return values
    ret = {}
    ret['repeat_type'] = MOTO_REPEAT_NONE
    ret['repeat_every'] = 0
    ret['repeat_day'] = 0
    ret['repeat_end'] = None
    ret['exceptions'] = []

    if len(rulenodes) == 0:
        return ret # no recursion
    elif len(rulenodes) > 1:
        # can't support multiple rules
        raise UnsupportedDataError('Unhandled recursion: too many rules')
    rulenode = rulenodes[0]

    freq = getXMLField(rulenode, 'Frequency').upper()

    interval = getXMLField(rulenode, 'Interval')
    if interval:
        ret['repeat_every'] = int(interval)

    def getSet(name, convfunc=int):
        """Convenience function, get a comma-separated set of values from XML."""
        val = getXMLField(rulenode, name)
        if not val:
            return None
        return set(map(convfunc, val.split(',')))

    bymonth = getSet('ByMonth')
    byyearday = getSet('ByYearDay')
    bymonthday = getSet('ByMonthDay')
    byday = getSet('ByDay', lambda s: s.upper())

    # convert byday strings to a list of tuples
    # first tuple elem is the "Nth" part, second is the day number
    # if no int part is present, the "Nth" value is 0
    if byday:
        byday_pairs = [(int(s[:-2] or "0"), VCAL_DAYS.index(s[-2:])) for s in byday]
        if len(byday) == 1:
            (byday_nth, byday_daynum) = byday_pairs[0]

    # compute the day and week number that the event falls in
    if eventdt.day % 7 == 0:
        eventweek = eventdt.day / 7
    else:
        eventweek = eventdt.day / 7 + 1

    # convenience variable for the tests below
    byallmonths = set(range(1, 12))

    # now test if the rule matches what we can represent
    if (freq == 'DAILY' and not byday and not bymonthday and not byyearday and not bymonth):
        ret['repeat_type'] = MOTO_REPEAT_DAILY
    elif (freq == 'WEEKLY' and not bymonthday and not byyearday and not bymonth):
        ret['repeat_type'] = MOTO_REPEAT_WEEKLY
        if byday:
            # the Nth specifier doesn't make sense on a byday value in a weekly rule
            # so we can ignore them
            byday_nums = [daynum for (_, daynum) in byday_pairs]
            if byday_nums != [eventdt.day]:
                ret['repeat_day'] = moto_weekdays_to_repeat_day(byday_nums, eventdt.day)
    elif (freq == 'MONTHLY' and not byday and (not bymonthday or bymonthday == set([eventdt.day]))
            and not byyearday and (not bymonth or bymonth == byallmonths)):
        if extended_format:
            ret['repeat_type'] = MOTO_REPEAT_EXTENDED_MONTHLY_DATE
        else:
            ret['repeat_type'] = MOTO_REPEAT_SIMPLE_MONTHLY_DATE
    elif (freq == 'MONTHLY' and byday and len(byday) == 1 and not bymonthday
            and not byyearday and (not bymonth or bymonth == byallmonths)):
        if extended_format:
            ret['repeat_type'] = MOTO_REPEAT_EXTENDED_MONTHLY_DAY
        else:
            ret['repeat_type'] = MOTO_REPEAT_SIMPLE_MONTHLY_DAY
        if not (byday_daynum == eventdt.weekday() and byday_nth == eventweek):
            ret['repeat_day'] = moto_monthday_to_repeat_day(byday_nth, byday_daynum)
    elif (freq == 'YEARLY' and not byday and (not bymonthday or bymonthday == set([eventdt.day]))
            and not byyearday and (not bymonth or bymonth == set([eventdt.month]))):
        if extended_format:
            ret['repeat_type'] = MOTO_REPEAT_EXTENDED_YEARLY_DATE
        else:
            ret['repeat_type'] = MOTO_REPEAT_SIMPLE_YEARLY_DATE
    elif (freq == 'YEARLY' and extended_format and byday and len(byday == 1)
            and (not bymonthday or bymonthday == set([eventdt.day]))
            and not byyearday and (not bymonth or bymonth == set([eventdt.month]))):
        ret['repeat_type'] = MOTO_REPEAT_EXTENDED_YEARLY_DAY
        if not (byday_daynum == eventdt.weekday() and byday_nth == eventweek):
            ret['repeat_day'] = moto_monthday_to_repeat_day(byday_nth, byday_daynum)
    else:
        raise UnsupportedDataError('Unhandled recursion: cannot represent rule')

    # phew, looks like we have something the phone can represent
    # now we need to work out the exceptions and see if this event still occurs

    # create an rrule object for this rule, and an rrule set
    ruleset = rrule.rruleset()
    ruleset.rrule(xml_rrule_to_dateutil(rulenode, eventdt))

    # if the rule ends, get its last occurrence
    if getXMLField(rulenode, 'Until') or getXMLField(rulenode, 'Count'):
        ret['repeat_until'] = ruleset[-1]

    # what events would happen if there were no exceptions?
    now = datetime.now()
    all_occurrences = ruleset.between(now, now + RRULE_FUTURE)

    # add in the exception dates and rules (if any)
    for node in exdates:
        for e in getElementsByTagNames(node, 'Content'):
            ruleset.exdate(dateutil.parser.parse(getXMLText(e)))

    for node in exrules:
        ruleset.exrule(xml_rrule_to_dateutil(node, eventdt))

    # are there any future occurrences of this event if we consider exceptions?
    if ruleset.after(now) == None:
        raise UnsupportedDataError('Unhandled recursion: no future occurrence')

    # what events will happen with exceptions
    excepted_occurrences = frozenset(ruleset.between(now, now + RRULE_FUTURE))

    # work out which events don't happen
    for num in range(all_occurrences):
        if all_occurrences[num] not in excepted_occurrences:
            ret['exceptions'].append(num)

    return ret

def moto_rrule_to_xml(doc, eventdt, repeat_type, exceptions, extended_format,
                      repeat_every=None, repeat_day=None, repeat_end=None):
    """Convert a Motorola-format recurrence to the corresponding XML description.

    Arguments correspond to the return values of xml_rrule_to_moto above.

    Returns a list of XML node objects.
    """

    # compute the week number that the event falls in
    if eventdt.day % 7 == 0:
        weeknum = eventdt.day / 7
    else:
        weeknum = eventdt.day / 7 + 1

    e = doc.createElement('RecurrenceRule')
    if repeat_type == MOTO_REPEAT_DAILY:
        appendXMLTag(doc, e, 'Frequency', 'DAILY')
        rule = rrule.rrule(rrule.DAILY,
                           dtstart=eventdt, interval=repeat_every, until=repeat_end)
    elif repeat_type == MOTO_REPEAT_WEEKLY:
        appendXMLTag(doc, e, 'Frequency', 'WEEKLY')
        if repeat_day:
            repeat_days = moto_repeat_day_to_weekdays(repeat_day, eventdt.weekday()).sort()
            appendXMLTag(doc, e, 'ByDay', ','.join([VCAL_DAYS[n] for n in repeat_days]))
        else:
            repeat_days = []
        rule = rrule.rrule(rrule.WEEKLY, byweekday=repeat_days,
                           dtstart=eventdt, interval=repeat_every, until=repeat_end)
    elif ((not extended_format and repeat_type == MOTO_REPEAT_SIMPLE_MONTHLY_DATE)
          or (extended_format and repeat_type == MOTO_REPEAT_EXTENDED_MONTHLY_DATE)):
        appendXMLTag(doc, e, 'Frequency', 'MONTHLY')
        appendXMLTag(doc, e, 'ByMonthDay', str(eventdt.day))
        rule = rrule.rrule(rrule.MONTHLY, bymonthday=eventdt.day,
                           dtstart=eventdt, interval=repeat_every, until=repeat_end)
    elif ((not extended_format and repeat_type == MOTO_REPEAT_SIMPLE_MONTHLY_DAY)
          or (extended_format and repeat_type == MOTO_REPEAT_EXTENDED_MONTHLY_DAY)):
        appendXMLTag(doc, e, 'Frequency', 'MONTHLY')
        if repeat_day:
            (repeat_nth, repeat_daynum) = moto_repeat_day_to_monthday(repeat_day)
        else: # default to repeating on the same week/day as the event
            repeat_nth = weeknum
            repeat_daynum = eventdt.weekday()
        appendXMLTag(doc, e, 'ByDay', '%d%s' % (repeat_nth, VCAL_DAYS[repeat_daynum]))
        rule = rrule.rrule(rrule.MONTHLY, byweekday=rrule.weekdays[repeat_daynum](repeat_nth),
                           dtstart=eventdt, interval=repeat_every, until=repeat_end)
    elif ((not extended_format and repeat_type == MOTO_REPEAT_SIMPLE_YEARLY_DATE)
          or (extended_format and repeat_type == MOTO_REPEAT_EXTENDED_YEARLY_DATE)):
        appendXMLTag(doc, e, 'Frequency', 'YEARLY')
        appendXMLTag(doc, e, 'ByMonth', str(eventdt.month))
        appendXMLTag(doc, e, 'ByMonthDay', str(eventdt.day))
        rule = rrule.rrule(rrule.YEARLY, bymonth=eventdt.month, bymonthday=eventdt.day,
                           dtstart=eventdt, interval=repeat_every, until=repeat_end)
    elif extended_format and repeat_type == MOTO_REPEAT_EXTENDED_YEARLY_DAY:
        appendXMLTag(doc, e, 'Frequency', 'YEARLY')
        appendXMLTag(doc, e, 'ByMonth', str(eventdt.month))
        if repeat_day:
            (repeat_nth, repeat_daynum) = moto_repeat_day_to_monthday(repeat_day)
        else: # default to repeating on the same week/day as the event
            repeat_nth = weeknum
            repeat_daynum = eventdt.weekday()
        appendXMLTag(doc, e, 'ByDay', '%d%s' % (repeat_nth, VCAL_DAYS[repeat_daynum]))
        rule = rrule.rrule(rrule.YEARLY, bymonth=eventdt.month,
                           byweekday=rrule.weekdays[repeat_daynum](repeat_nth),
                           dtstart=eventdt, interval=repeat_every, until=repeat_end)

    if repeat_every:
        appendXMLTag(doc, e, 'Interval', str(repeat_every))

    if repeat_end:
        appendXMLTag(doc, e, 'Until', format_time(repeat_end, VCAL_DATETIME))

    if e.hasChildNodes():
        ret = [e]
    else:
        return []

    if exceptions != []:
        # work out which dates the exceptions correspond to
        e = doc.createElement('ExceptionDateTime')
        e.setAttribute('Value', 'DATE')
        for exnum in exceptions:
            appendXMLTag(doc, e, 'Content', format_time(rule[exnum], VCAL_DATE))
        if e.hasChildNodes():
            ret.append(e)

    return ret

def xmlevent_to_moto_simple(node, event):
    """Parse an XML event (node), setting the fields on a PhoneEvent object (event)

    Only the common fields between PhoneEventSimple and Extended are handled.
    Recursion-related fields are ignored.

    This function raises UnsupportedDataError for:
     * events before year 2000 (my phone doesn't allow them)
    """

    def getField(tagname, subtag='Content'):
        """utility function for the XML processing below"""
        return getXMLField(node, tagname, subtag)

    event.name = getField('Summary')

    event.eventdt = parse_ical_time(getField('DateStarted'))
    if event.eventdt.year < 2000:
        raise UnsupportedDataError('Event is too old')

    if node.getElementsByTagName('Duration') != []:
        duration = node.getElementsByTagName('Duration')[0]
        def toint(numstr):
            """Convert a string to an integer, unless it's None, in which case return 0."""
            if numstr is None:
                return 0
            else:
                return int(numstr)
        weeks = toint(getXMLField(duration, 'Weeks'))
        days = toint(getXMLField(duration, 'Days'))
        hours = toint(getXMLField(duration, 'Hours'))
        mins = toint(getXMLField(duration, 'Minutes'))
        secs = toint(getXMLField(duration, 'Seconds'))
        event.duration = timedelta(weeks * 7 + days, (hours * 60 + mins) * 60 + secs)
    else:
        endstr = getField('DateEnd')
        if endstr is not None:
            event.duration = parse_ical_time(endstr) - event.eventdt
        else:
            # no duration or end specified, assume whole-day or no duration
            if isinstance(event.eventdt, date):
                event.duration = timedelta(1)
            else:
                event.duration = timedelta(0)

    # for some reason I don't understand, the phone only allows events
    # longer than a day if the time flag is set. pander to this by forcing
    # such events to start at midnight local time
    if isinstance(event.eventdt, date) and event.duration > timedelta(1):
        local_midnight = datetime_time(0, 0, 0, 0, dateutil.tz.tzlocal())
        event.eventdt = datetime.combine(event.eventdt, local_midnight)

    triggerstr = getField('Alarm', 'AlarmTrigger')
    if triggerstr is None:
        event.alarmdt = None
    else:
        if triggerstr.startswith('-P') or triggerstr.startswith('P'):
            offset = parse_ical_duration(triggerstr)
            event.alarmdt = event.eventdt + offset
        else:
            event.alarmdt = parse_ical_time(triggerstr)


class UnsupportedDataError(Exception):
    """Exception raised by PhoneEntry classes when the data cannot be stored."""
    def __init__(self, msg):
        Exception.__init__(self)
        self.msg = msg
    def __str__(self):
        return self.msg


class PhoneComms:
    """Functions for directly accessing the phone.

    "device" may be either a path to a local device node, or a bluetooth MAC.
    """
    def __init__(self, device):
        self.devstr = device
        self.__calendar_open = False
        self.__calendar_locking_required = True
        self.__fd = self.__btsock = None
        self.max_events = None
        self.num_events = None
        self.event_name_len = None
        self.event_max_exceptions = None
        self.min_contact_pos = None
        self.max_contact_pos = None
        self.contact_data_len = None
        self.contact_name_len = None
        self.extended_events_supported = None

    def connect(self):
        """Connect to the phone and initiate communication.

        Returns True on success, False if already connected.
        """
        if (self.__fd or self.__btsock):
            return False # already connected

        if BT_MAC_RE.match(self.devstr):
            assert(USE_BLUETOOTH_MODULE,
                   "MAC address specified, but pybluez module is not available")

            # search for the port to use on the device
            port = BT_DEFAULT_CHANNEL
            found = bluetooth.find_service(name=BT_SERVICE_NAME, address=self.devstr)
            if found:
                assert(found[0]['protocol'] == 'RFCOMM')
                port = found[0]['port']
            self.__btsock = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
            self.__btsock.connect((self.devstr, port))
        else:
            self.__fd = os.open(self.devstr, os.O_RDWR)
            if USE_TTY_MODULE:
                tty.setraw(self.__fd)
            else:
                opensync.trace(opensync.TRACE_INTERNAL, 'tty module not present, unable to set raw mode')

        # reset the phone and send it a bunch of init strings
        # I don't know what the mode numbers are, but AT&F only works in MODE=0, and
        # people have reported that other commands only work in MODE=2, hence this...
        self.__do_cmd('AT+MODE=0') # change mode
        self.__do_cmd('AT&F')      # reset to factory defaults
        self.__do_cmd('AT+MODE=2') # change mode again
        self.__do_cmd('ATE0Q0V1')  # echo off, result codes off, verbose results

        # find out if calendar locking is required/supported by this phone
        try:
            self.__do_cmd('AT+MDBL=0')
        except opensync.Error:
            self.__calendar_locking_required = False

        # use UCS2 encoding for data values
        # this is an older version of UTF16 where every char is two bytes long
        # the phone implements it by sending us 2 hex chars per byte, ie. 4 per char
        self.__do_cmd('AT+CSCS="UCS2"')

        (maxevs, numevs, namelen, max_except, extended) = self.read_event_params()
        self.max_events = maxevs
        self.num_events = numevs
        self.event_name_len = namelen
        self.event_max_exceptions = max_except
        self.extended_events_supported = extended

        (minpos, maxpos, contactlen, namelen) = self.read_contact_params()
        self.min_contact_pos = minpos
        self.max_contact_pos = maxpos
        self.contact_data_len = contactlen
        self.contact_name_len = namelen

        return True

    def disconnect(self):
        """Disconnect from the phone."""
        self.close_calendar()
        if self.__fd:
            os.close(self.__fd)
            self.__fd = None
        if self.__btsock:
            self.__btsock.close()
            self.__btsock = None

    def read_serial(self):
        """read the phone's serial number (IMEI)"""
        data = self.__do_cmd('AT+CGSN')
        return self.__parse_results('CGSN', data)[0][0]

    def read_version(self):
        """read the phone's software version number"""
        data = self.__do_cmd('AT+CGMR')
        return self.__parse_results('CGMR', data)[0][0]

    def read_model(self):
        """read the phone's hardware model number"""
        data = self.__do_cmd('AT+CGMM')
        for s in self.__parse_results('CGMM', data)[0]:
            if s.startswith("MODEL="):
                return s.split("=", 1)[1]
        return None

    def read_time(self):
        """read the phone's current date & time"""
        data = self.__do_cmd('AT+CCLK?')
        return self.__parse_results('CCLK', data)[0][0]

    def read_features(self):
        """find out what features this phone supports"""
        data = self.__do_cmd('AT+MAID?')
        return map(int, self.__parse_results('MAID', data)[0])

    def open_calendar(self):
        """Open the calendar.

        This "locks" out the phone's own UI from accessing the data.
        """
        if self.__calendar_locking_required and not self.__calendar_open:
            self.__do_cmd('AT+MDBL=1')
        self.__calendar_open = True

    def close_calendar(self):
        """Close the calendar."""
        if self.__calendar_locking_required and self.__calendar_open:
            self.__do_cmd('AT+MDBL=0')
        self.__calendar_open = False

    def read_event_params(self):
        """Read calendar/datebook parameters.

        Returns: maximum number of events,
                 number of events currently stored,
                 length of title/name field,
                 maximum number of event exceptions
                 true/false if extended event format is supported
        """
        self.open_calendar()
        data = self.__do_cmd('AT+MDBR=?') # read event parameters
        results = self.__parse_results('MDBR', data)[0]
        return results[:4] + [len(results) >= 10]

    def read_events(self):
        """Read all events on the phone.

        Returns a list of (event data, exceptions) tuples.
        """
        self.open_calendar()

        # read entries from the phone until we've seen all of them
        ret = []
        pos = 0
        while pos < self.max_events and len(ret) < self.num_events:
            end = min(pos + ENTRIES_PER_READ - 1, self.max_events - 1)
            data = self.__do_cmd('AT+MDBR=%d,%d' % (pos, end))

            # first parse all the exceptions for each event
            exceptions = {}
            for (expos, exnum, extype) in self.__parse_results('MDBRE', data):
                if extype != 1: # haven't seen anything else
                    raise opensync.Error('unexpected exception type %d' % extype)
                if not exceptions.has_key(expos):
                    exceptions[expos] = []
                exceptions[expos].append(exnum)
            # ...then add them into the event data
            for evdata in self.__parse_results('MDBR', data):
                ret.append((evdata, exceptions.get(evdata[0], [])))
            pos += ENTRIES_PER_READ
        return ret

    def write_event(self, data, exceptions):
        """Write a single event to the phone.

        Uses pos specified in event (overwriting anything on the phone).
        """
        self.open_calendar()
        pos = data[0]
        self.delete_event(pos)
        # HACK: only the name of the event (data[1]) should be unicode
        for n in range(2, len(data)):
            if type(data[n]) == types.UnicodeType:
                data[n] = data[n].encode('ascii')
        self.__do_cmd('AT+MDBW=' + self.__to_cmd_str(data))
        for expos in exceptions:
            self.__do_cmd('AT+MDBWE=%d,%d,1' % (pos, expos))

    def delete_event(self, pos):
        """delete the event at a specific position"""
        self.open_calendar()
        self.__do_cmd('AT+MDBWE=%d,0,0' % pos)

    def read_categories(self):
        """Get list of category IDs/names for the phonebook."""
        self.close_calendar()
        # open phone memory
        self.__do_cmd('AT+CPBS="ME"')

        # read groups
        data = self.__do_cmd('AT+MPGR=?')
        (mingroup, maxgroup) = self.__parse_results('MPGR', data)[0][0]
        ret = []
        pos = mingroup
        while pos <= maxgroup:
            end = min(pos + ENTRIES_PER_READ - 1, maxgroup)
            data = self.__do_cmd('AT+MPGR=%d,%d' % (pos, end))
            ret.extend(self.__parse_results('MPGR', data))
            pos += ENTRIES_PER_READ

        return ret

    def read_contact_params(self):
        """Read phonebook parameters.

        Returns: minimum position,
                 maximum position,
                 length of contact field,
                 length of name field
        """
        # open phone memory
        self.close_calendar()
        self.__do_cmd('AT+CPBS="ME"')

        # read parameters
        data = self.__do_cmd('AT+CPBR=?')
        results = self.__parse_results('CPBR', data)[0]
        ((minpos, maxpos), numberlen, namelen) = results
        return (minpos, maxpos, numberlen, namelen)

    def read_contacts(self):
        """read a list of all contacts in the phonebook"""

        # open phone memory
        self.close_calendar()
        self.__do_cmd('AT+CPBS="ME"')

        # check usage
        data = self.__do_cmd('AT+CPBS?')
        result = self.__parse_results('CPBS', data)[0]
        assert(result[0] == 'ME')

        if len(result) >= 3:
            # newer phones gives us three values
            (memtype, inuse, maxused) = result[:3]
            assert(memtype == 'ME')
            assert(self.max_contact_pos - self.min_contact_pos + 1 <= maxused)
        else:
            # older phones we don't seem to know how many entries are in use,
            # so have to read all of them :(
            inuse = self.max_contact_pos - self.min_contact_pos

        # read entries from the phone until we've seen all of them
        ret = []
        pos = self.min_contact_pos
        while pos <= self.max_contact_pos and len(ret) < inuse:
            end = min(pos + ENTRIES_PER_READ - 1, self.max_contact_pos)
            data = self.__do_cmd('AT+MPBR=%d,%d' % (pos, end))
            ret.extend(self.__parse_results('MPBR', data))
            pos += ENTRIES_PER_READ
        return ret

    def write_contact(self, data):
        """write a single contact to the position specified in the data list"""
        self.close_calendar()
        # HACK: the email/number and birthday (index 1&23) must not be unicode
        for n in [1, 23]:
            if len(data) > n and type(data[n]) == types.UnicodeType:
                data[n] = data[n].encode('ascii')
        self.__do_cmd('AT+MPBW=' + self.__to_cmd_str(data))

    def delete_contact(self, pos):
        """delete the contact at a given position"""
        self.close_calendar()
        self.__do_cmd('AT+MPBW=%d' % pos)

    def __readchar(self):
        """read a single character from the phone device"""
        if self.__fd:
            return os.read(self.__fd, 1)
        elif self.__btsock:
            return self.__btsock.recv(1)

    def __readline(self):
        """read the next line of text from the phone"""
        ret = ''
        c = self.__readchar()
        while c == '\r' or c == '\n':
            c = self.__readchar()
        while c != '\r' and c != '\n' and c != '':
            ret += c
            c = self.__readchar()
        opensync.trace(opensync.TRACE_SENSITIVE, '<-- ' + ret)
        if c == '': # EOF, shouldn't happen
            raise opensync.Error('Unexpected EOF talking to phone', opensync.ERROR_IO_ERROR)
        return ret

    def __do_cmd(self, cmd):
        """Send a command to the phone and wait for its response.

        If it succeeds, return lines as a list; otherwise raise an exception.
        """
        opensync.trace(opensync.TRACE_SENSITIVE, '--> ' + cmd)
        ret = []
        cmd = cmd + '\r'
        if self.__fd:
            os.write(self.__fd, cmd)
        elif self.__btsock:
            self.__btsock.send(cmd)
        line = self.__readline()
        while line != 'OK' and line != 'ERROR':
            if line != 'RING':
                ret.append(line)
            line = self.__readline()
        if line == 'OK':
            return ret
        else:
            raise opensync.Error("Error in phone command '%s'" % cmd, opensync.ERROR_IO_ERROR)

    def __parse_results(self, restype, lines):
        """Extract results of the specified type from a list of reply lines.

        Returns a list of results, where each result is itself a list of values.
        String values have quote characters stripped, and are decoded from UCS2.
        Numeric values are converted to integers.
        Range values (eg: (0-10)) are converted to lists of integers.
        """
        # FIXME: this code is fragile and too complex
        ret = []
        prefix = '+' + restype + ': '
        for line in lines:
            if line.startswith(prefix):
                # remove prefix
                line = line[len(prefix):]

                # split line into comma-separated values, observing quotes
                parts = []
                nextpart = ''
                inquote = inbracket = False
                for c in line:
                    if c == '"':
                        inquote = not inquote
                        nextpart += c
                    elif (c == '(' or c == ')') and not inquote:
                        inbracket = not inbracket
                        nextpart += c
                    elif c == ',' and not (inquote or inbracket):
                        parts.append(nextpart)
                        nextpart = ''
                    else:
                        nextpart += c
                parts.append(nextpart)

                # convert quoted values to strings, everything else to integers
                valparts = []
                for part in parts:
                    if part == '':
                        valparts.append('')
                    elif part[0] == '"':
                        assert(part[-1] == '"')
                        valparts.append(part[1:-1])
                    elif part[0] == '(':
                        # parse a range string like '(1-10,45,50-60)'
                        assert(part[-1] == ')')
                        subparts = part[1:-1].split(',')
                        ranges = [map(int, s.split('-', 1)) for s in subparts]
                        if len(ranges) == 1:
                            ranges = ranges[0]
                        valparts.append(ranges)
                    elif len(part) >= 4 and part[0] == '0':
                        # this looks like a UCS2-encoded string
                        valparts.append(part.decode('hex').decode('utf_16_be'))
                    else:
                        try:
                            part = int(part)
                        except ValueError:
                            pass # leave as a string
                        valparts.append(part)
                ret.append(valparts)
        return ret

    def __to_cmd_str(self, vals):
        """Convert different typed data to a string that can be used in a phone
        write command (ie. MPBW or MDBW."""
        def make_placeholder(val):
            """Return a single placeholder based on the given value's type."""
            t = type(val)
            if t == types.IntType:
                return '%d'
            elif t == types.StringType or val == '':
                return '"%s"'
            elif t == types.UnicodeType:
                return '%s'
            else:
                assert(False, 'unexpected type %s' % str(t))

        def convert_val(val):
            """Convert a value to an alternate representation, if needed."""
            if type(val) == types.UnicodeType:
                # convert to the phone's idea of UCS2
                # FIXME: this breaks in the case of surrogate pairs, but I doubt
                # they'll turn up in PIM data, and it's not clear what the phone
                # actually does support as its character set
                return val.encode('utf_16_be').encode('hex').upper()
            else:
                return val

        return ','.join(map(make_placeholder, vals)) % tuple(map(convert_val, vals))



class PhoneEntry:
    """(abstract) base class representing an event/contact entry

    data is kept roughly as it is stored in the phone
    """
    def __init__(self):
        pass

    def get_objtype(self):
        """return the opensync object type string"""
        raise NotImplementedError

    def generate_uid(self):
        """Return (part of) a UID that holds the positions used by this entry"""
        raise NotImplementedError

    def num_pos(self):
        """Return the number of positions occupied by this entry."""
        raise NotImplementedError

    def get_pos(self):
        """Return the positions occupied by this entry."""
        raise NotImplementedError

    def set_pos(self, positions):
        """Set the positions occupied by this entry."""
        raise NotImplementedError

    def write(self, comms):
        """given an instance of PhoneComms, write this entry to the phone"""
        raise NotImplementedError

    def hash_data(self):
        """return a list of entry data in a predictable format, for hashing"""
        raise NotImplementedError


class PhoneEventSimple(PhoneEntry):
    """Class representing the events roughly as stored in the phone.
    This class should not be instantiated directly, use one of
    PhoneEventSimpleMoto or PhoneEventSimpleXML depending on the data format.

    class members:
       pos:         integer position/slot in phone memory
       name:        name/summary of event
       eventdt:     datetime or date object (if no time is set) for event
       duration:    timedelta object for duration of event
       alarmdt:     datetime object or None, for alarm time
       repeat_type: integer 0-5 for repeating events
       exceptions:  list of occurrences (0-based) which do not actually happen
    """
    def __init__(self):
        PhoneEntry.__init__(self)
        self.pos = self.name = self.eventdt = self.duration = None
        self.alarmdt = None
        self.repeat_type = MOTO_REPEAT_NONE
        self.exceptions = []

    @staticmethod
    def capabilities():
        """Return the OpenSync XML capabilities supported by this object."""
        caps = """
            <event>
                <Alarm />
                <DateEnd />
                <DateStarted />
                <Duration />
                <ExceptionDateTime />
                <RecurrenceRule />
                <Summary />
            </event>
        """
        return caps

    def get_objtype(self):
        """return the opensync object type string"""
        return "event"

    def generate_uid(self):
        """Return (part of) a UID that holds the positions used by this entry"""
        return str(self.pos)

    @staticmethod
    def unpack_uid(uid):
        """Unpack the string returned by generate_uid to a list of positions."""
        return [int(uid)]

    def num_pos(self):
        """Return the number of positions occupied by this entry."""
        return 1

    def get_pos(self):
        """Return the positions occupied by this entry."""
        return [self.pos]

    def set_pos(self, positions):
        """Set the positions occupied by this entry."""
        assert(len(positions) == 1)
        self.pos = positions[0]

    def write(self, comms):
        """given an instance of PhoneComms, write this entry to the phone"""
        self.__truncate_fields(comms)
        comms.write_event(self.to_moto(), self.exceptions)

    def hash_data(self):
        """return a list of entry data in a predictable format, for hashing"""
        return self.to_moto()

    def __truncate_fields(self, comms):
        """enforce length limits by truncating data"""
        self.name = self.name[:comms.event_name_len]
        self.exceptions = self.exceptions[:comms.event_max_exceptions]

    def to_moto(self):
        """generate motorola event-data list"""
        if isinstance(self.eventdt, datetime):
            timeflag = 1
            datestr = format_time(self.eventdt, PHONE_DATE)
            timestr = format_time(self.eventdt, PHONE_TIME)
        else:
            timeflag = 0
            datestr = self.eventdt.strftime(PHONE_DATE)
            timestr = MOTO_INVALID_TIME
        if self.alarmdt:
            alarmflag = 1
            alarmdatestr = format_time(self.alarmdt, PHONE_DATE)
            alarmtimestr = format_time(self.alarmdt, PHONE_TIME)
        else:
            alarmflag = 0
            alarmdatestr = MOTO_INVALID_DATE
            alarmtimestr = MOTO_INVALID_TIME

        duration = int(self.duration.days) * 24 * 60
        duration += int(self.duration.seconds) / 60
        return [self.pos, self.name, timeflag, alarmflag, timestr, datestr,
                duration, alarmtimestr, alarmdatestr, self.repeat_type]

    def to_xml(self, include_rrule=True):
        """Returns OpenSync XML representation of this event.

        If include_rrule is False, any RecurrenceRule and Exception nodes are
        omitted. This is a hack for use by the PhoneEventExtended class only.
        """
        impl = xml.dom.minidom.getDOMImplementation()
        doc = impl.createDocument(None, 'event', None)
        top = doc.documentElement

        if self.alarmdt:
            alarm = doc.createElement('Alarm')
            appendXMLTag(doc, alarm, 'AlarmAction', 'DISPLAY')
            appendXMLTag(doc, alarm, 'AlarmDescription', self.name)
            alarmtime = self.alarmdt.strftime(VCAL_DATETIME)
            appendXMLTag(doc, alarm, 'AlarmTrigger', alarmtime)
            top.appendChild(alarm)

        e = doc.createElement('DateEnd')
        endtime = self.eventdt + self.duration
        if isinstance(self.eventdt, datetime):
            dtend = endtime.strftime(VCAL_DATETIME)
        else:
            dtend = endtime.strftime(VCAL_DATE)
            e.setAttribute('Value', 'DATE')
        appendXMLTag(doc, e, 'Content', dtend)
        top.appendChild(e)

        e = doc.createElement('DateStarted')
        if isinstance(self.eventdt, datetime):
            dtstart = self.eventdt.strftime(VCAL_DATETIME)
        else:
            dtstart = self.eventdt.strftime(VCAL_DATE)
            e.setAttribute('Value', 'DATE')
        appendXMLTag(doc, e, 'Content', dtstart)
        top.appendChild(e)

        e = doc.createElement('Summary')
        appendXMLTag(doc, e, 'Content', self.name)
        top.appendChild(e)

        if include_rrule:
            nodes = moto_rrule_to_xml(doc, self.eventdt, self.repeat_type, self.exceptions, False)
            for node in nodes:
                insertXMLNode(top, node)

        return doc


class PhoneEventSimpleMoto(PhoneEventSimple):
    """Constructor for the PhoneEventSimple object with data in Motorola format"""
    def __init__(self, data, exceptions):
        """grab stuff out of the list of values from the phone"""
        PhoneEventSimple.__init__(self)
        assert(type(data) == list and len(data) >= 10)
        self.pos = data[0]
        self.name = data[1]
        timeflag = data[2]
        alarmflag = data[3]
        timestr = data[4]
        datestr = data[5]
        self.duration = timedelta(0, 0, 0, 0, data[6])
        alarmtime = data[7]
        alarmdate = data[8]
        self.repeat_type = data[9]
        self.exceptions = exceptions
        self.exceptions.sort() # just in case

        if timeflag:
            self.eventdt = parse_moto_time(datestr, timestr)
        else:
            self.eventdt = parse_moto_time(datestr)

        if alarmflag:
            self.alarmdt = parse_moto_time(alarmdate, alarmtime)
        else:
            self.alarmdt = None


class PhoneEventSimpleXML(PhoneEventSimple):
    """Constructor for the PhoneEventSimple object with data in OpenSync XML format"""
    def __init__(self, xmldata):
        """Parse XML event data."""
        PhoneEventSimple.__init__(self)
        doc = xml.dom.minidom.parseString(xmldata)
        node = doc.getElementsByTagName('event')[0]

        xmlevent_to_moto_simple(node, self)

        rrules = node.getElementsByTagName('RecurrenceRule')
        exdates = node.getElementsByTagName('ExceptionDateTime')
        exrules = node.getElementsByTagName('ExceptionRule')

        rule = xml_rrule_to_moto(rrules, exdates, exrules, self.eventdt, False)
        if (rule['repeat_every'] not in [0, 1] or rule['repeat_day'] != 0 or rule['repeat_end']):
            raise UnsupportedDataError("Recursion rule not supported by simple event format")

        self.repeat_type = rule['repeat_type']
        self.exceptions = rule['exceptions']


class PhoneEventExtended(PhoneEventSimple):
    """Class representing the extended version of the event data supported by
    newer phone models.

    This class should not be instantiated directly, use one of
    PhoneEventExtendedMoto or PhoneEventExtendedXML depending on the data format.

    class members (in addition to those defined by PhoneEventSimple):
       event_type:  integer 0-15
       location:    string
       note:        string (stores XML description field)
       repeat_every: integer
       repeat_day:  integer
       repeat_end:  date object or None, for repeat end date
    """
    def __init__(self):
        PhoneEventSimple.__init__(self)
        self.event_type = MOTO_EVENT_TYPE_NONE
        self.location = self.note = ''
        self.repeat_every = self.repeat_day = 0
        self.repeat_end = None

    @staticmethod
    def capabilities():
        """Return the OpenSync XML capabilities supported by this object."""
        caps = """
            <event>
                <Alarm />
                <Categories />
                <DateEnd />
                <DateStarted />
                <Description />
                <Duration />
                <ExceptionDateTime />
                <Location />
                <RecurrenceRule />
                <Summary />
            </event>
        """
        return caps

    def __truncate_fields(self, comms):
        """enforce length limits by truncating data"""
        self.name = self.name[:comms.event_name_len]
        self.exceptions = self.exceptions[:comms.event_max_exceptions]
        # FIXME: other string fields

    def to_moto(self):
        """Generate motorola event-data list."""

        # call simple version to generate base data
        data = PhoneEventSimple.to_moto(self)

        # HACK: duration (field 6) is unused in the extended format
        data[6] = ''

        if isinstance(self.eventdt, datetime):
            enddatestr = format_time(self.eventdt + self.duration, PHONE_DATE)
            endtimestr = format_time(self.eventdt + self.duration, PHONE_TIME)
        else:
            enddatestr = (self.eventdt + self.duration).strftime(PHONE_DATE)
            endtimestr = MOTO_INVALID_TIME

        if self.repeat_end:
            repeatendstr = format_time(self.repeat_end, PHONE_DATE)
        else:
            repeatendstr = MOTO_INVALID_DATE

        return data + [endtimestr, enddatestr, '', self.event_type,
                       self.location, self.note, 501, self.repeat_every,
                       self.repeat_day, '', repeatendstr]

    def to_xml(self):
        """Returns OpenSync XML representation of this event."""

        # get simple version of XML, without rrule included
        doc = PhoneEventSimple.to_xml(self, False)
        top = doc.documentElement

        if self.event_type != MOTO_EVENT_TYPE_NONE:
            e = doc.createElement('Categories')
            appendXMLTag(doc, e, 'Category', MOTO_EVENT_TYPES[self.event_type])
            insertXMLNode(top, e)

        e = doc.createElement('Location')
        appendXMLTag(doc, e, 'Content', self.location)
        insertXMLNode(top, e)

        e = doc.createElement('Summary')
        appendXMLTag(doc, e, 'Content', self.note)
        insertXMLNode(top, e)

        nodes = moto_rrule_to_xml(doc, self.eventdt, self.repeat_type,
                    self.exceptions, True, self.repeat_every, self.repeat_day,
                    self.repeat_end)
        for node in nodes:
            insertXMLNode(top, node)

        return doc


class PhoneEventExtendedMoto(PhoneEventExtended):
    """Constructor for the PhoneEventExtended object with data in Motorola format"""
    def __init__(self, data, exceptions):
        """grab stuff out of the list of values from the phone"""
        PhoneEventExtended.__init__(self)
        assert(type(data) == list and len(data) >= 21)

        # reuse simple constructor to initialise common fields 0-9
        PhoneEventSimpleMoto.__init__(self, data, exceptions)

        endtime = data[10]
        enddate = data[11]
        # alarm sound (field 12) is currently ignored
        self.event_type = data[13]
        self.location = data[14]
        self.note = data[15]
        # field 16 is unknown, and apparently always 501
        self.repeat_every = data[17]
        self.repeat_day = data[18]
        # field 19 is unknown, and apparently always empty
        if data[20] != MOTO_INVALID_DATE:
            self.repeat_end = parse_moto_time(data[20])

        if isinstance(self.eventdt, datetime):
            enddt = parse_moto_time(enddate, endtime)
        else:
            enddt = parse_moto_time(enddate)
        self.duration = enddt - self.eventdt


class PhoneEventExtendedXML(PhoneEventExtended):
    """Constructor for the PhoneEventExtended object with data in XML format."""
    def __init__(self, xmldata):
        PhoneEventExtended.__init__(self)

        doc = xml.dom.minidom.parseString(xmldata)
        node = doc.getElementsByTagName('event')[0]

        xmlevent_to_moto_simple(node, self)

        self.location = getXMLField(node, 'Location', 'Content')
        self.note = getXMLField(node, 'Summary', 'Content')

        catname = getXMLField(node, 'Categories', 'Category')
        if catname:
            catname = catname.lower()
        self.event_type = REVERSE_MOTO_EVENT_TYPES.get(catname, MOTO_EVENT_TYPE_NONE)

        rrules = node.getElementsByTagName('RecurrenceRule')
        exdates = node.getElementsByTagName('ExceptionDateTime')
        exrules = node.getElementsByTagName('ExceptionRule')
        rule = xml_rrule_to_moto(rrules, exdates, exrules, self.eventdt, True)
        self.repeat_type = rule['repeat_type']
        self.repeat_every = rule['repeat_every']
        self.repeat_day = rule['repeat_day']
        self.repeat_end = rule['repeat_end']
        self.exceptions = rule['exceptions']


class PhoneContact(PhoneEntry):
    """Class representing the contacts roughly as stored in the phone.

    PhoneContact represents the common fields of a logical identity that may be
    split across several entries in the phone book for different contact types.

    class members:
       children:        list of PhoneContactChild objects, that contain
                        attributes unique to each entry
       name:            name of this entry
       categorynum:     category to which this entry belongs
       firstlast_enabled/
       firstlast_index: subfield information about name
                        (where FirstName and LastName are within the string)
            enabled stores:
                        0=firstname lastname, 1=lastname firstname, 255=unknown
            index stores:
                        0-based index of second field (ignored if enabled=255)
       birthday:        date object for birthday
       nickname:        nickname
    """
    def __init__(self):
        PhoneEntry.__init__(self)
        self.children = []
        self.name = self.categorynum = None
        self.firstlast_enabled = MOTO_INVALID
        self.firstlast_index = 0
        self.birthday = self.nickname = None

    @staticmethod
    def capabilities():
        """Return the OpenSync XML capabilities supported by this object."""
        caps = """
            <contact>
                <Address />
                <Birthday />
                <Categories />
                <EMail />
                <FormattedName />
                <Name>
                    <LastName />
                    <FirstName />
                </Name>
                <Nickname />
                <Telephone />
            </contact>
            """
        return caps

    def get_objtype(self):
        """return the opensync object type string"""
        return "contact"

    def generate_uid(self):
        """Return (part of) a UID that holds the positions used by this entry"""
        return ','.join([str(c.pos) for c in self.children])

    @staticmethod
    def unpack_uid(uid):
        """Unpack the string returned by generate_uid to a list of positions."""
        return map(int, uid.split(','))

    def num_pos(self):
        """Return the number of positions occupied by this entry."""
        return len(self.children)

    def get_pos(self):
        """Return the positions occupied by this entry."""
        return [c.pos for c in self.children]

    def set_pos(self, positions):
        """Set the positions occupied by this entry."""
        assert(len(positions) == len(self.children))
        for (p, c) in zip(positions, self.children):
            c.pos = p

    def write(self, comms):
        """given an instance of PhoneComms, write this entry to the phone"""
        self.__truncate_fields(comms)
        for child in self.children:
            child.truncate_fields(comms)
            comms.write_contact(child.to_moto())

    def hash_data(self):
        """return a list of entry data in a predictable format, for hashing"""
        ret = []
        for child in self.children:
            ret.extend(list(child.to_moto()))
        return ret

    def merge(self, other):
        """(Try to) merge this contact with another, adding it to our children.

        Returns True iff it succeeds.
        """
        if (other.name == self.name
            and other.nickname == self.nickname
            and other.categorynum == self.categorynum):

            assert(len(other.children) == 1)
            newchild = other.children[0]
            other.children = []
            newchild.parent = self

            # avoid duplicating addresses
            if newchild.address:
                for c in self.children:
                    if c.address == newchild.address:
                        if newchild.primaryflag:
                            c.address = None
                        else:
                            newchild.address = None

            self.children.append(newchild)

            # copy any other info we don't have
            if other.birthday and not self.birthday:
                self.birthday = other.birthday
            if (other.firstlast_enabled != MOTO_INVALID
                and self.firstlast_enabled == MOTO_INVALID):
                self.firstlast_enabled = other.firstlast_enabled
                self.firstlast_index = other.firstlast_index

            return True

        return False

    def to_xml(self, categories):
        """Returns OpenSync XML representation of this contact."""
        impl = xml.dom.minidom.getDOMImplementation()
        doc = impl.createDocument(None, 'contact', None)
        top = doc.documentElement

        if self.birthday:
            bdaystr = format_time(self.birthday, VCAL_DATE)
            e = doc.createElement('Birthday')
            appendXMLTag(doc, e, 'Content', bdaystr)
            top.appendChild(e)

        e = doc.createElement('Categories')
        appendXMLTag(doc, e, 'Category', categories[self.categorynum])
        top.appendChild(e)

        e = doc.createElement('FormattedName')
        appendXMLTag(doc, e, 'Content', self.name)
        top.appendChild(e)

        e = doc.createElement('Name')
        if self.firstlast_enabled == MOTO_INVALID:
            # FIXME: have to guess at name split, this is what opensync does:
            appendXMLTag(doc, e, 'LastName', self.name)
        else:
            first = self.name[:self.firstlast_index].strip()
            last = self.name[self.firstlast_index:].strip()
            if self.firstlast_enabled:
                appendXMLTag(doc, e, 'LastName', first)
                appendXMLTag(doc, e, 'FirstName', last)
            else:
                appendXMLTag(doc, e, 'LastName', last)
                appendXMLTag(doc, e, 'FirstName', first)
        top.appendChild(e)

        if self.nickname != '':
            e = doc.createElement('Nickname')
            appendXMLTag(doc, e, 'Content', self.nickname)
            top.appendChild(e)

        for child in self.children:
            for newnode in child.child_xml(doc):
                # insert the child to maintain alphabetic order of the nodes
                # unfortunately the merger code requires this order
                insertXMLNode(top, newnode)

        return doc

    def __truncate_fields(self, comms):
        """enforce field length limits by truncating data"""
        self.name = self.name[:comms.contact_name_len]


class PhoneContactMoto(PhoneContact):
    """PhoneContact object for contacts created from phone data.

    Creates a contact with a single child."""
    def __init__(self, data):
        """grab stuff out of the list of values from the phone"""
        PhoneContact.__init__(self)
        assert(type(data) == list and len(data) >= 24)
        self.name = data[3]
        self.categorynum = data[9]
        self.firstlast_enabled = data[11]
        self.firstlast_index = data[12]
        assert(data[14] == 0) # unknown?
        assert(data[15] == 0) # unknown?
        self.nickname = data[22]
        if data[23] == '':
            self.birthday = None
        else:
            self.birthday = parse_moto_time(data[23])
        if len(data) >= 25:
            assert(data[24] == '') # unknown? old phones don't have it

        self.children.append(PhoneContactChildMoto(self, data))


class PhoneContactXML(PhoneContact):
    """PhoneContact object for contacts created from XML data."""
    def __init__(self, data, revcategories):
        PhoneContact.__init__(self)
        doc = xml.dom.minidom.parseString(data)
        contact = doc.getElementsByTagName('contact')[0]

        def getField(tagname, subtag='Content'):
            """utility function for the XML processing below"""
            return getXMLField(contact, tagname, subtag)

        # handle the name and formatted name fields
        self.name = getField('FormattedName')
        if self.name:
            # FIXME: cheesy attempt at taking apart the FormattedName
            last = getField('Name', 'LastName')
            if last:
                lastidx = self.name.find(last)
                if lastidx == 0:
                    first = getField('Name', 'FirstName')
                    if first:
                        firstidx = self.name.find(first)
                        if firstidx != -1:
                            self.firstlast_enabled = 1
                            self.firstlast_index = firstidx
                elif lastidx != -1:
                    self.firstlast_enabled = 0
                    self.firstlast_index = lastidx
        else:
            # compute FormattedName from Name
            nameparts = [getField('Name', p) for p in XML_NAME_PARTS]
            nameparts = [p for p in nameparts if p]
            self.name = ' '.join(nameparts)
            self.firstlast_enabled = 0
            self.firstlast_index = self.name.index(getField('Name', 'LastName'))

        catname = getField('Categories', 'Category')
        if catname:
            catname = catname.lower()
        self.categorynum = revcategories.get(catname, MOTO_CATEGORY_DEFAULT)
        self.nickname = getField('Nickname')
        bdaystr = getField('Birthday')
        if bdaystr:
            self.birthday = parse_ical_time(bdaystr)
        else:
            self.birthday = None

        # NOW WE PROCESS ALL THE CONTACT INFO AND CREATE CHILDREN

        # process telephone numbers and email addresses
        # create a list [(contacttype, contact, is_pref)]
        contacts = []
        seen_pref = False # have we already seen a preferred contact?
        for elt in getElementsByTagNames(doc, set(['Telephone', 'EMail']), []):
            content = getXMLField(elt, 'Content')
            if elt.tagName == 'Telephone':
                # filter out any illegal characters from the phone number
                content = filter(lambda c: c in TEL_NUM_DIGITS, content)
                ical_types = elt.getAttribute('Location').split(';')
                ical_types.extend(elt.getAttribute('Type').split(';'))
                is_pref = elt.hasAttribute('Preferred') and elt.getAttribute('Preferred') in ['1', 'true']
                moto_type = MOTO_CONTACT_DEFAULT
                for t in ical_types:
                    t = t.lower()
                    if VCARD_CONTACT_TYPES.has_key(t):
                        moto_type = VCARD_CONTACT_TYPES[t]
                        break
            else: # email
                is_pref = False
                moto_type = MOTO_CONTACT_EMAIL
            contacts.append((moto_type, content, is_pref and not seen_pref))
            seen_pref = seen_pref or is_pref

        # if none of the contacts were preffered, make one so
        if not seen_pref:
            # arbitrarily mark the first non-email contact as preferred
            for i in range(len(contacts)):
                (moto_type, content, _) = contacts[i]
                if moto_type != MOTO_CONTACT_EMAIL:
                    contacts[i] = (moto_type, content, True)
                    break
            else:
                # no non-email contacts, so just make the first email preferred
                (moto_type, content, _) = contacts[0]
                contacts[0] = (moto_type, content, True)

        # process addresses, create a hash from contacttype to address
        # addresses that don't map to moto contact types are dropped
        addresses = {}
        for adr in doc.getElementsByTagName('Address'):
            address = [getXMLField(adr, p) for p in XML_ADDRESS_PARTS]
            ical_types = adr.getAttribute('Location').split(';')
            # XXX: HACK: if it has no type, assume it's a home address
            if ical_types == [] or ical_types == ['']:
                ical_types = ['Home']
            for t in ical_types:
                t = t.lower()
                if VCARD_ADDRESS_TYPES.has_key(t):
                    moto_type = VCARD_ADDRESS_TYPES[t]
                    addresses[moto_type] = address
                    break

        # create a child for each telephone/address pair or email
        # addresses for which there is no phone number are dropped
        for (moto_type, contact, is_pref) in contacts:
            if moto_type == MOTO_CONTACT_EMAIL:
                addr = None
            else:
                addr = addresses.get(moto_type)
            child = PhoneContactChild(self, contact, moto_type, is_pref, addr)
            self.children.append(child)

        if self.children == []:
            # this entry has no contact data
            raise UnsupportedDataError('No telephone or email data')


class PhoneContactChild:
    """Class representing the contacts roughly as stored in the phone.

    This class stores data unique to each entry in the phone, and not
    shared between "linked" entries. That data is in the parent PhoneContact.

    class members:
       parent:          parent PhoneContact object
       pos:             position of this entry in the phone book
       contact:         contact data (number or email address)
       contacttype:     integer representing contact type (home/work/etc, email)
       numtype:         one of MOTO_NUMTYPE_*
       voicetag:        flag if voice-dial tag is set for this entry
       ringerid:        ringer ID for this entry
       primaryflag:     flag set on the "primary" contact for a given identity
       profile_icon:    profile icon number
       picture_path:    path to picture for this entry (on the phone's VFS)
       address:         None, or list of address parts
    """
    def __init__(self, parent, contact, contacttype, primary, address):
        self.parent = parent
        self.pos = None
        self.voicetag = 0
        self.ringerid = MOTO_INVALID
        self.profile_icon = MOTO_INVALID
        self.picture_path = ''
        self.contact = contact
        self.contacttype = contacttype
        self.primaryflag = primary
        if contacttype in [MOTO_CONTACT_EMAIL, MOTO_CONTACT_MAILINGLIST]:
            self.numtype = MOTO_NUMTYPE_UNKNOWN
        elif contact[0] == '+':
            self.numtype = MOTO_NUMTYPE_INTL
        else:
            self.numtype = MOTO_NUMTYPE_LOCAL
        self.address = address

    def truncate_fields(self, comms):
        """enforce length limits by truncating data"""
        self.contact = self.contact[:comms.contact_data_len]
        # FIXME: address parts need truncating

    def to_moto(self):
        """Generate motorola contact-data list."""
        if self.parent.birthday:
            birthdaystr = format_time(self.parent.birthday, PHONE_DATE)
        else:
            birthdaystr = ''
        if self.parent.nickname:
            nickname = self.parent.nickname
        else:
            nickname = ''
        if self.address:
            # remove None parts
            address = []
            for part in self.address:
                if part:
                    address.append(part)
                else:
                    address.append('')
            (street1, street2, city, state, postcode, country) = address
        else:
            street1 = street2 = city = state = postcode = country = ''
        return [self.pos, self.contact, self.numtype, self.parent.name,
                self.contacttype, self.voicetag, self.ringerid, 0,
                int(self.primaryflag), self.parent.categorynum,
                self.profile_icon, self.parent.firstlast_enabled,
                self.parent.firstlast_index, self.picture_path,
                0, 0, street2, street1, city, state, postcode, country,
                nickname, birthdaystr]

    def child_xml(self, doc):
        """Return XML nodes for this child's data."""
        ret = []

        if self.address:
            e = doc.createElement('Address')
            for (part, val) in zip(XML_ADDRESS_PARTS, self.address):
                appendXMLTag(doc, e, part, val)
            if e.hasChildNodes():
                if MOTO_ADDRESS_TYPES.has_key(self.contacttype):
                    e.setAttribute('Location', MOTO_ADDRESS_TYPES[self.contacttype])
                ret.append(e)

        if self.contacttype == MOTO_CONTACT_EMAIL:
            e = doc.createElement('EMail')
        elif self.contacttype == MOTO_CONTACT_MAILINGLIST:
            # the 'contact' is a space-separated list of other contact positions
            assert(False, "mailing lists aren't handled yet, sorry") # FIXME: implement mailing lists
        else:
            e = doc.createElement('Telephone')
            if MOTO_PHONE_CONTACT_LOCATIONS.has_key(self.contacttype):
                e.setAttribute('Location', MOTO_PHONE_CONTACT_LOCATIONS[self.contacttype])
            elif MOTO_PHONE_CONTACT_TYPES.has_key(self.contacttype):
                e.setAttribute('Type', MOTO_PHONE_CONTACT_TYPES[self.contacttype])
            if self.primaryflag:
                e.setAttribute('Preferred', 'true')
        appendXMLTag(doc, e, 'Content', self.contact)
        ret.append(e)

        return ret


class PhoneContactChildMoto(PhoneContactChild):
    """Phone contact child object for children created from phone data."""
    def __init__(self, parent, data):
        assert(type(data) == list and len(data) >= 24)
        adr = (data[17], data[16], data[18], data[19], data[20], data[21])
        PhoneContactChild.__init__(self, parent, data[1], data[4], data[8], adr)
        self.pos = data[0]
        self.numtype = data[2]
        self.voicetag = data[5]
        self.ringerid = data[6]
        self.profile_icon = data[10]
        self.picture_path = data[13]


class PosAllocator:
    """Position-allocator class.

    Remembers which positions in a set are used/free, and allocates new ones.
    """
    def __init__(self, objtype, minpos, maxpos):
        self.objtype = objtype
        self.minpos = minpos
        self.maxpos = maxpos
        self.used = set([])

    def mark_used(self, positions):
        """Mark all the positions in the given list as used."""
        for pos in positions:
            if pos in range(self.minpos, self.maxpos + 1):
                self.used.add(pos)

    def mark_free(self, positions):
        """Mark all the positions in the given list as free."""
        for pos in positions:
            if pos in range(self.minpos, self.maxpos + 1):
                self.used.remove(pos)

    def alloc(self, num_required=1):
        """Allocate free positions for a new entry."""
        ret = []
        i = self.minpos
        while len(ret) < num_required:
            while i in self.used:
                i += 1
            if i > self.maxpos:
                raise opensync.Error('No %s positions free' % self.objtype)
            self.used.add(i)
            ret.append(i)
        return ret


class PhoneAccess:
    """Grab-bag class of utility functions.

     Interfaces between PhoneComms/PhoneEntry classes and SyncClass below.
     """
    def __init__(self, comms, info):
        self.comms = comms
        self.serial = None
        self.positions = {}
        self.categories = {}
        self.revcategories = {}
        self.objformats = {}

        # find ObjFormat objects for our types
        for objtype in SUPPORTED_OBJTYPES:
            formatstr = "xmlformat-%s-doc" % objtype
            formatobj = info.format_env.find_objformat(formatstr)
            if not formatobj:
                raise opensync.Error('object format %s unknown' % formatstr)
            self.objformats[objtype] = formatobj

    def connect(self):
        """Connect to the phone and setup our data structures."""
        if not self.comms.connect():
            return # already connected
        self.serial = self.comms.read_serial()

        # check that the phone supports the features we need
        features = self.comms.read_features()
        for (bit, desc) in REQUIRED_FEATURES:
            if not features[bit]:
                raise opensync.Error(desc + ' feature not present', opensync.ERROR_NOT_SUPPORTED)

        # read current time on the phone, check if it matches our local time
        # FIXME: allow the user to configure a different timezone for the phone
        timestr = self.comms.read_time()[:-3]
        phone_now = time.strptime(timestr, '%y/%m/%d,%H:%M:%S')
        local_now = time.localtime()
        if abs(time.mktime(phone_now) - time.mktime(local_now)) > 60 * 30:
            msg = ("ERROR: Phone appears to be in a different timezone!\n"
                   + "Phone time is %s" % time.strftime('%c', phone_now))
            raise opensync.Error(msg)

        # initialise the position allocators
        self.positions['event'] = PosAllocator('event', 0, self.comms.max_events - 1)
        min_contact = max(self.comms.min_contact_pos, MOTO_QUICKDIAL_ENTRIES)
        self.positions['contact'] = PosAllocator('contact', min_contact, self.comms.max_contact_pos)

        # initialise the category mappings
        self.__init_categories()

    def disconnect(self):
        """Disconnect from the phone."""
        self.comms.disconnect()

    def list_changes(self, objtype):
        """Return a list of change objects for all entries of the given type."""

        if objtype == 'contact':
            # sort contacts by name, and attempt to merge adjacent ones
            entries = [PhoneContactMoto(d) for d in self.comms.read_contacts()]
            entries.sort(key=lambda c: (c.name, c.get_pos()))
            i = 0
            while i < (len(entries) - 1):
                if entries[i].merge(entries[i + 1]):
                    del entries[i + 1]
                else:
                    i += 1
        elif objtype == 'event':
            entries = [self.__event_from_moto(d, x) for (d, x) in self.comms.read_events()]
        else:
            assert(False, 'Unknown objtype %s' % objtype)

        ret = []
        for entry in entries:
            if objtype == 'contact':
                xmldata = entry.to_xml(self.categories).toxml()
            else:
                xmldata = entry.to_xml().toxml()
            data = opensync.Data(xmldata, self.objformats[objtype])
            data.objtype = objtype
            change = opensync.Change()
            change.data = data
            change.uid = self.__generate_uid(entry)
            change.hash = self.__gen_hash(entry)
            ret.append(change)
            self.positions[objtype].mark_used(entry.get_pos())
        return ret

    def delete_entry(self, objtype, uid):
        """Delete an event with the given objtype and UID

        Returns True on success, False otherwise.
        """
        positions = self.__uid_to_pos(uid, objtype)
        for pos in positions:
            if objtype == 'event':
                self.comms.delete_event(pos)
            elif objtype == 'contact':
                self.comms.delete_contact(pos)
        self.positions[objtype].mark_free(positions)
        return True

    def update_entry(self, change, context):
        """Update an entry or add a new one, from the OSyncChange object.

        Returns True on success, False otherwise.
        """
        objtype = change.objtype
        if change.objformat.name != self.objformats[objtype].name:
            raise opensync.Error("unhandled data format " + change.objformat.name, opensync.ERROR_NOT_SUPPORTED)
        try:
            if objtype == 'event':
                if self.comms.extended_events_supported:
                    entry = PhoneEventExtendedXML(change.data.data)
                else:
                    entry = PhoneEventSimpleXML(change.data.data)
            elif objtype == 'contact':
                entry = PhoneContactXML(change.data.data, self.revcategories)
        except UnsupportedDataError, e:
            err = opensync.Error("%s is unsupported (%s), ignored" % (change.uid, str(e)), opensync.ERROR_NOT_SUPPORTED)
            context.report_osyncwarning(err)
            # we have an entry that can't be stored on the phone
            # if its modified, we've seen it before, so delete it
            # otherwise just ignore it
            if change.changetype == opensync.CHANGE_TYPE_MODIFIED:
                change.changetype = opensync.CHANGE_TYPE_DELETED
                change.data = None
                return self.delete_entry(objtype, change.uid)
            else:
                return False
        if change.changetype == opensync.CHANGE_TYPE_ADDED:
            # allocate positions for the new entry
            positions = self.positions[objtype].alloc(entry.num_pos())
            entry.set_pos(positions)
        else:
            positions = self.__uid_to_pos(change.uid, objtype)
            # check if the number of positions required has changed
            pos_diff = entry.num_pos() - len(positions)
            if pos_diff > 0:
                # need to allocate more positions
                positions.extend(self.positions[objtype].alloc(pos_diff))
                positions.sort()
            elif pos_diff < 0:
                # need to free some positions
                free_positions = positions[pos_diff:]
                positions = positions[:pos_diff]
                for pos in free_positions:
                    assert(objtype == 'contact')
                    self.comms.delete_contact(pos)
                self.positions[objtype].mark_free(free_positions)
            entry.set_pos(positions)
        entry.write(self.comms)
        change.uid = self.__generate_uid(entry)
        change.hash = self.__gen_hash(entry)

        return True

    def __init_categories(self):
        """Initialise a hash and reverse hash of category IDs."""
        # for each category we get: pos,name,other junk
        for data in self.comms.read_categories():
            catid, catname = data[:2]
            self.categories[catid] = catname
            self.revcategories[catname.lower()] = catid

    def __event_from_moto(self, data, exceptions):
        """Construct the appropriate event object from the phone data."""
        if self.comms.extended_events_supported:
            return PhoneEventExtendedMoto(data, exceptions)
        else:
            return PhoneEventSimpleMoto(data, exceptions)

    @staticmethod
    def __gen_hash(entry):
        """generate hash for the opensync hashtable, md5 of all the data"""
        m = md5.new()
        for item in entry.hash_data():
            m.update(unicode(item).encode('utf7'))
        return m.hexdigest()

    @staticmethod
    def __generate_uid(entry):
        """Generate a UID for an entry, that encodes the objtype and the position."""
        return ("moto-%s-%s" % (entry.get_objtype(), entry.generate_uid()))

    @staticmethod
    def __uid_to_pos(uid, objtype):
        """Reverse of the generate_uid function above.

        Also checks that the objtype matches.
        """
        try:
            moto, uid_objtype, lastpart = uid.split('-', 2)
            assert(moto == "moto")
        except (ValueError, AssertionError):
            raise opensync.Error("Invalid UID: " + uid)

        if uid_objtype != objtype:
            raise opensync.Error("UID %s doesn't match objtype %s", uid, objtype)

        if objtype == "event":
            return PhoneEventSimple.unpack_uid(lastpart)
        elif objtype == "contact":
            return PhoneContact.unpack_uid(lastpart)
        else:
            raise opensync.Error("Unsupported objtype: " + uid)


class MotoSink(opensync.ObjTypeSinkCallbacks):
    """Event synchronisation class used by OpenSync."""

    def __init__(self, objtype, info, access):
        opensync.ObjTypeSinkCallbacks.__init__(self, objtype)
        self.objtype = objtype
        self.sink.add_objformat("xmlformat-%s-doc" % objtype)
        self.access = access
        hashpath = os.path.join(info.configdir, "%s-hash.db" % objtype)
        self.hashtable = opensync.HashTable(hashpath, objtype)

    def connect(self, info, ctx):
        """Connect to the phone."""
        self.access.connect()
        anchorpath = os.path.join(info.configdir, 'anchor.db')
        if not opensync.anchor_compare(anchorpath, 'serial', self.access.serial):
            self.sink.slowsync = True

    def get_changes(self, info, ctx):
        """Report all OSyncChange objects for entries on the phone."""
        if self.sink.slowsync:
            self.hashtable.reset()
        for change in self.access.list_changes(self.objtype):
            self.hashtable.report(change.uid)
            change.changetype = self.hashtable.get_changetype(change.uid, change.hash)
            if change.changetype != opensync.CHANGE_TYPE_UNMODIFIED:
                self.hashtable.update_hash(change.changetype, change.uid, change.hash)
                ctx.report_change(change)
        for uid in self.hashtable.get_deleted():
            change = opensync.Change()
            change.uid = uid
            change.changetype = opensync.CHANGE_TYPE_DELETED
            ctx.report_change(change)
            self.hashtable.update_hash(opensync.CHANGE_TYPE_DELETED, uid, None)

    def commit(self, info, ctx, change):
        """Write a change to the phone."""
        if change.objtype != self.objtype:
            raise opensync.Error('unsupported objtype %s' % change.objtype, opensync.ERROR_NOT_SUPPORTED)
        if change.changetype == opensync.CHANGE_TYPE_DELETED:
            success = self.access.delete_entry(change.objtype, change.uid)
        elif change.changetype == opensync.CHANGE_TYPE_MODIFIED:
            old_uid = change.uid
            success = self.access.update_entry(change, ctx)
            # if the UID has changed, we need to tell our hashtable that
            # the old one was deleted, to keep it consistent
            if (success and old_uid != change.uid):
                self.hashtable.update_hash(opensync.CHANGE_TYPE_DELETED, old_uid, None)
        else:
            success = self.access.update_entry(change, ctx)
        if success:
            self.hashtable.update_hash(change.changetype, change.uid, change.hash)

    def sync_done(self, info, ctx):
        """Called when a sync completes successfully."""
        anchorpath = os.path.join(info.configdir, 'anchor.db')
        opensync.anchor_update(anchorpath, 'serial', self.access.serial)

    def disconnect(self, info, ctx):
        """Called to disconnect from the phone."""
        self.access.disconnect()


def parse_config(configstr):
    """Parse the config data and return the device string."""
    try:
        doc = xml.dom.minidom.parseString(configstr)
    except:
        raise opensync.Error('failed to parse config data', opensync.ERROR_MISCONFIGURATION)

    ret = getXMLField(doc, 'device').strip()
    if not ret:
        raise opensync.Error('device not specified in config file', opensync.ERROR_MISCONFIGURATION)
    return ret

def initialize(info):
    """Called by python-module plugin wrapper, registers sync classes."""
    comms = PhoneComms(parse_config(info.config))
    access = PhoneAccess(comms, info)
    for objtype in SUPPORTED_OBJTYPES:
        info.add_objtype(MotoSink(objtype, info, access).sink)

def discover(info):
    """Called by python-module wrapper, discovers capabilities of device."""
    version = opensync.Version()
    version.plugin = "moto-sync"

    # HACK HACK, grab the comms object out of the initialised sink
    comms = info.nth_objtype(0).callback_obj.access.comms
    comms.connect()

    version.softwareversion = str(comms.read_version())
    version.modelversion = str(comms.read_model())

    if comms.extended_events_supported:
        event_caps = PhoneEventExtended.capabilities()
    else:
        event_caps = PhoneEventSimple.capabilities()

    comms.disconnect()

    all_caps = """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
        <capabilities>
        %s
        %s
        </capabilities>
        """ % (PhoneContact.capabilities(), event_caps)

    info.capabilities = opensync.capabilities_parse(all_caps)
    info.version = version

    # for now, all objtypes are supported on all devices
    for sink in info.objtypes:
        sink.available = True

def get_sync_info(plugin):
    """Called by python-module plugin wrapper, returns plugin metadata."""
    plugin.name = "moto-sync"
    plugin.longname = "Motorola synchronisation plugin"
    plugin.description = ("OpenSync plugin to synchronise phone book and"
        " calendar entries on a locally-connected Motorola mobile phone using"
        " extended AT commands. Please see the README file for configuration"
        " instructions and a list of supported models.")
    plugin.config_type = opensync.PLUGIN_NEEDS_CONFIGURATION
