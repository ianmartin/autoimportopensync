"""
 opensync plugin for syncing to a Motorola mobile phone
 HIGHLY EXPERIMENTAL, USE AT YOUR OWN RISK!
"""

# Copyright (C) 2006  Andrew Baumann <andrewb@cse.unsw.edu.au>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of version 2 of the GNU General Public License
#  as published by the Free Software Foundation.

__revision__ = "$Id$"

import sys, os, types, traceback, md5, time, calendar, re
import xml.dom.minidom
from datetime import date, datetime, timedelta, tzinfo
import dateutil.parser, dateutil.rrule, dateutil.tz
import opensync

try:
    import tty # this module is only present on Unix
except ImportError:
    pass

# debug options:
DEBUG_OUTPUT = True # if enabled, prints interaction with the phone to stdout
WRITE_ENABLED = True # if disabled, prevents changes from being made to phone

# object types supported by this plugin
SUPPORTED_OBJTYPES = ['event', 'contact']

# my phone doesn't like it if you read more than this many entries at a time
ENTRIES_PER_READ = 15

# time and date formats
PHONE_TIME = '%H:%M'
PHONE_DATE = '%m-%d-%Y' # yuck!
VCAL_DATETIME = '%Y%m%dT%H%M%S'
VCAL_DATE = '%Y%m%d'

# days of the week in vcal parlance
VCAL_DAYS = ['MO', 'TU', 'WE', 'TH', 'FR', 'SA', 'SU']

# repeat types in the calendar
MOTO_REPEAT_NONE = 0
MOTO_REPEAT_DAILY = 1
MOTO_REPEAT_WEEKLY = 2
MOTO_REPEAT_MONTHLY_DATE = 3
MOTO_REPEAT_MONTHLY_DAY = 4
MOTO_REPEAT_YEARLY = 5

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

# mapping from phone's contact type to vcard number types
MOTO_CONTACT_TYPES = {
    0: 'work',
    1: 'home',
    2: 'voice',
    3: 'cell',
    4: 'fax',
    5: 'pager',
    11: 'voice' # shows up as "other", seen on a RAZR V3x
}

# reverse of the above (almost): mapping from vcard to phone's contact type
VCARD_CONTACT_TYPES = {
    'work':  0,
    'home':  1,
    'voice': 2,
    'cell':  3,
    'car':   3,
    'pcs':   3,
    'fax':   4,
    'pager': 5,
}

# (dodgy) mapping from address types to motorola contact types
# can also have dom/intl/postal/parcel... these don't really make sense here
VCARD_ADDRESS_TYPES = {
    'work':  0,
    'home':  1,
}

# reverse of the above
MOTO_ADDRESS_TYPES = {
    0: 'work',
    1: 'home',
}

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

# logical order of the fields in structured XML data
XML_NAME_PARTS = 'Prefix FirstName Additional LastName Suffix'.split()
XML_ADDRESS_PARTS = ('Street ExtendedAddress'
                     + ' City Region PostalCode Country').split()

# legal characters in telephone numbers
TEL_NUM_DIGITS = set('+0123456789')

# how far into the future to process exceptions for recurring events
RRULE_FUTURE = timedelta(365) # 1 year


def getElementsByTagNames(parent, tagnames, ret):
    """Like DOM's getElementsByTagName, but allow multiple tag names."""
    for node in parent.childNodes:
        if (node.nodeType == xml.dom.minidom.Node.ELEMENT_NODE
            and node.tagName in tagnames):
            ret.append(node)
        getElementsByTagNames(node, tagnames, ret)
    return ret

def getXMLField(doc, tagname, subtag=None):
    """Returns text in a given XML tag, or '' if not set.

    The XML structure that it looks for is:
        <tagname><subtag>text here</subtag></tagname>.
    It always returns the data in the first subtag of the first tag,
    repeated tags are ignored.
    """
    elts = doc.getElementsByTagName(tagname)
    if elts == []:
        return ''
    elt = elts[0]
    if subtag:
        children = elt.getElementsByTagName(subtag)
        if children == []:
            return ''
        elt = children[0]
    return getXMLText(elt)

def getXMLText(elt):
    """Returns all text within a given XML node."""
    textnodes = filter(lambda n: n.nodeType == n.TEXT_NODE, elt.childNodes)
    return ''.join(map(lambda n: n.data, textnodes))

def appendXMLChild(doc, parent, tag, text):
    """Append a child tag with supplied text content to the parent node."""
    if text and text != '':
        e = doc.createElement(tag)
        e.appendChild(doc.createTextNode(text.encode('utf8')))
        parent.appendChild(e)

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
DURATION_REGEX = re.compile(r'([-+]?)P(?:%s|%s)$'
                            % (WEEKS_PART, DATETIME_PART))

def parse_ical_duration(s):
    """Parse an ical duration string, returning a timedelta object."""
    try:
        match = DURATION_REGEX.match(s)
        sign, weeks, days, hours, minutes, seconds = match.groups()
        if weeks:
            value = timedelta(weeks=int(weeks))
        else:
            value = timedelta(days=int(days or 0),
                                hours=int(hours or 0),
                                minutes=int(minutes or 0),
                                seconds=int(seconds or 0))
        if sign == '-':
            value = -value
        return value
    except:
        raise ValueError('Invalid iCalendar duration: %s' % s)

def parse_ical_time(s):
    d = dateutil.parser.parse(s)
    if len(s) == 8: # just a date
        assert(d.hour == d.minute == d.second == 0)
        return d.date()
    else:
        return d

def format_time(dt, format):
    """convert datetime object into local time and format as a string"""
    if isinstance(dt, datetime) and dt.utcoffset() == timedelta(0):
        # FIXME: assumes local timezone
        t = time.localtime(calendar.timegm(dt.utctimetuple()))
        return time.strftime(format, t)
    else:
        # already in localtime
        return dt.strftime(format)

def convert_rrule(rulenodes, exdates, exrules, eventdt):
    """Process the recursion rules. Given lists of RecurrenceRule nodes,
    exception dates, exception rules, and the event's date/time,
    returns the motorola recurrence type and a list of exceptions.
    
    The general approach we take is: if the recursion can be represented
    by the phone's data structure, we convert it, otherwise we ignore the
    rule completely (to avoid the event showing up at incorrect times).
    
    FIXME: this code doesn't yet handle all the tricky parts of RRULE
    specifications (such as BYSETPOS)
    """
    
    # can't support multiple rules
    if len(rulenodes) != 1:
        return (MOTO_REPEAT_NONE, [])
    rulenode = rulenodes[0]

    # build hash of rule parts
    rules = {}
    for rule in map(getXMLText, rulenode.getElementsByTagName('Rule')):
        key, val = rule.split('=', 1)
        key = key.lower()
        if key[:1] == 'by':
            val = set(val.split(','))
        rules[key] = val

    # extract the parts
    assert(rules.has_key('freq')) # required by RFC2445
    freq = rules['freq'].lower()
    bymonth = rules.get('bymonth')
    byweekno = rules.get('byweekno')
    byyearday = rules.get('byyearday')
    bymonthday = rules.get('bymonthday')
    byday = rules.get('byday')

    # fail the conversion if any of these are set
    if (rules.has_key('byhour') or rules.has_key('byminute')
        or rules.has_key('bysecond') or rules.has_key('bysetpos')
        or rules.get('interval', '1') != '1'):
        return (MOTO_REPEAT_NONE, [])

    # compute the day and week number that the event falls in
    eventday = VCAL_DAYS[eventdt.weekday()]
    if eventdt.day % 7 == 0:
        eventweek = eventdt.day / 7
    else:
        eventweek = eventdt.day / 7 + 1

    # some convenience variables for the tests below
    byeventweekday = set(['%d%s' % (eventweek, eventday)])
    byalldays = set(VCAL_DAYS)
    byallmonths = set(range(1, 12))

    # now test if the rule matches what we can represent
    if (freq == 'daily' and (not byday or byday == set(VCAL_DAYS))
        and not bymonthday and not byyearday and not byweekno
        and not bymonth):
        moto_type = MOTO_REPEAT_DAILY
    elif (freq == 'weekly' and (not byday or byday == set([eventday]))
            and not bymonthday and not byyearday and not byweekno and
            not bymonth):
        moto_type = MOTO_REPEAT_WEEKLY
    elif (freq == 'monthly' and not byday
            and (not bymonthday or bymonthday == set([eventdt.day]))
            and not byyearday and not byweekno
            and (not bymonth or bymonth == byallmonths)):
        moto_type = MOTO_REPEAT_MONTHLY_DATE
    elif (freq == 'monthly' and byday == byeventweekday and not bymonthday
            and not byyearday and not byweekno
            and (not bymonth or bymonth == byallmonths)):
        moto_type = MOTO_REPEAT_MONTHLY_DAY
    elif (freq == 'yearly' and not byday
            and (not bymonthday or bymonthday == set([eventdt.day]))
            and not byyearday and not byweekno
            and (not bymonth or bymonth == set([eventdt.month]))):
        moto_type = MOTO_REPEAT_YEARLY
    else:
        return (MOTO_REPEAT_NONE, [])

    # phew, looks like we have something the phone can represent
    # now we need to work out the exceptions and see if this event still occurs

    # recombine the rule parts into a single string, and parse into an rruleset
    rulestr = ';'.join(map(getXMLText, rulenode.getElementsByTagName('Rule')))
    ruleset = dateutil.rrule.rrulestr(rulestr, dtstart=eventdt, forceset=True)

    # are there any future occurrences of this event?
    now = datetime.now()
    if ruleset.after(now) == None:
        return (MOTO_REPEAT_NONE, [])

    # what events would happen if there were no exceptions?
    all_occurrences = ruleset.between(now, now + RRULE_FUTURE)

    # add in the exception dates and rules (if any)
    for node in exdates:
        ruleset.exdate(dateutil.parser.parse(getXMLField(node, 'Content')))
    for node in exrules:
        ruleset.exrule(dateutil.rrule.rrulestr(getXMLField(node, 'Content')))

    # are there any future occurrences of this event if we consider exceptions?
    if ruleset.after(datetime.now()) == None:
        return (MOTO_REPEAT_NONE, [])

    # what events will happen with exceptions
    # use a set for better searching properties (FIXME: could be smarter)
    excepted_occurrences = set(ruleset.between(now, now + RRULE_FUTURE))

    # work out which events don't happen
    exceptions = []
    for num in range(all_occurrences):
        if all_occurrences[num] not in excepted_occurrences:
            exceptions.append(num)

    return (moto_type, exceptions)


class OpenSyncError(Exception):
    """Simple exception class carrying a message and an opensync error number.

    These errors are reported back to opensync by the stdexceptions decorator
    on SyncClass methods.
    """
    def __init__(self, msg, errnum=opensync.ERROR_GENERIC):
        self.msg = msg
        self.num = errnum
    def __str__(self):
        return self.msg
    def report(self, context):
        context.report_error(self.num, self.msg)


class PhoneComms:
    """Functions for directly accessing the phone."""
    def __init__(self, device):
        self.__calendar_open = False
        self.__fd = os.open(device, os.O_RDWR)

        try:
            tty.setraw(self.__fd)
        except NameError:
            print 'Warning: tty module not present, unable to set raw mode'

        # reset the phone and send it a bunch of init strings
        self.__do_cmd('AT&F')      # reset to factory defaults
        self.__do_cmd('AT+MODE=0') # ?
        self.__do_cmd('ATE0Q0V1')  # echo off, result codes off, verbose results

        # use ISO 8859-1 encoding for data values, for easier debugging
        # FIXME: change to UCS2?
        self.__do_cmd('AT+CSCS="8859-1"')

    def __del__(self):
        try:
            self.close_calendar()
            os.close(self.__fd)
        except: # we might not have successfully opened this
            pass

    def read_serial(self):
        """read the phone's serial number (IMEI)"""
        data = self.__do_cmd('AT+CGSN')
        return self.__parse_results('CGSN', data)[0][0]

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
        if not self.__calendar_open:
            self.__do_cmd('AT+MDBL=1')
            self.__calendar_open = True

    def close_calendar(self):
        """Close the calendar."""
        if self.__calendar_open:
            self.__do_cmd('AT+MDBL=0')
            self.__calendar_open = False

    def read_events(self):
        """read the list of all events on the phone"""
        self.open_calendar()

        # read calendar/event parameters
        data = self.__do_cmd('AT+MDBR=?') # read event parameters
        res = self.__parse_results('MDBR', data)[0]
        (maxevents, numevents, titlelen, exmax, extypemax) = res

        # read entries from the phone until we've seen all of them
        ret = []
        pos = 0
        while pos < maxevents and len(ret) < numevents:
            end = min(pos + ENTRIES_PER_READ - 1, maxevents - 1)
            data = self.__do_cmd('AT+MDBR=%d,%d' % (pos, end))

            # first parse all the exceptions for each event
            exceptions = {}
            for (expos, exnum, extype) in self.__parse_results('MDBRE', data):
                if extype != 1: # haven't seen anything else
                    raise OpenSyncError('unexpected exception type %d' % extype)
                if not exceptions.has_key(expos):
                    exceptions[expos] = []
                exceptions[expos].append(exnum)
            # ...then add them into the event data
            for evdata in self.__parse_results('MDBR', data):
                evdata.append(exceptions.get(pos, []))
                ret.append(evdata)
            pos += ENTRIES_PER_READ
        return ret

    def write_event(self, evdata):
        """Write a single event to the phone.

        Uses pos specified in event (overwriting anything on the phone).
        """
        self.open_calendar()
        pos = evdata[0]
        self.delete_event(pos)
        exceptions = evdata[-1]
        data = evdata[:-1]
        placeholders = self.__make_placeholders(data)
        self.__do_cmd(('AT+MDBW=' + placeholders) % tuple(data), WRITE_ENABLED)
        for expos in exceptions:
            self.__do_cmd('AT+MDBWE=%d,%d,1' % (pos, expos), WRITE_ENABLED)

    def delete_event(self, pos):
        """delete the event at a specific position"""
        self.open_calendar()
        self.__do_cmd('AT+MDBWE=%d,0,0' % pos, WRITE_ENABLED)

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
        res = self.__parse_results('CPBR', data)[0]
        ((self.min_contact_pos, self.max_contact_pos), numberlen, namelen) = res
        return (self.min_contact_pos, self.max_contact_pos, numberlen, namelen)

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
        placeholders = self.__make_placeholders(data)
        self.__do_cmd(('AT+MPBW=' + placeholders) % tuple(data), WRITE_ENABLED)

    def delete_contact(self, pos):
        """delete the contact at a given position"""
        self.close_calendar()
        self.__do_cmd('AT+MPBW=%d' % pos, WRITE_ENABLED)

    def __readline(self):
        """read the next line of text from the phone"""
        ret = ''
        c = os.read(self.__fd, 1)
        while c == '\r' or c == '\n':
            c = os.read(self.__fd, 1)
        while c != '\r' and c != '\n' and c != '':
            ret += c
            c = os.read(self.__fd, 1)
        if DEBUG_OUTPUT:
            print ('<-- ' + ret)
        if c == '': # EOF, shouldn't happen
            raise OpenSyncError('Unexpected EOF talking to phone',
                                opensync.ERROR_IO_ERROR)
        return ret

    def __do_cmd(self, cmd, reallydoit=True):
        """Send a command to the phone and wait for its response.

        If it succeeds, return lines as a list; otherwise raise an exception.
        """
        cmd = cmd.encode('iso_8859_1')
        if DEBUG_OUTPUT:
            print ('--> ' + cmd)
        ret = []
        if reallydoit:
            os.write(self.__fd, cmd + "\r")
            line = self.__readline()
        else:
            line = 'OK'
        while line != 'OK' and line != 'ERROR':
            ret.append(line)
            line = self.__readline()
        if line == 'OK':
            return ret
        else:
            raise OpenSyncError("Error in phone command '%s'" % cmd,
                                opensync.ERROR_IO_ERROR)

    def __parse_results(self, restype, lines):
        """extract results from a list of reply lines"""
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
                    if part[0] == '"':
                        assert(part[-1] == '"')
                        valparts.append(part[1:-1].decode('iso_8859_1'))
                    elif part[0] == '(':
                        # parse a range string like '(1-10,45,50-60)'
                        assert(part[-1] == ')')
                        subparts = part[1:-1].split(',')
                        ranges = map(lambda s: map(int, s.split('-', 1)),
                                     subparts)
                        if len(ranges) == 1:
                            ranges = ranges[0]
                        valparts.append(ranges)
                    else:
                        try:
                            part = int(part)
                        except ValueError:
                            pass # leave as a string
                        valparts.append(part)
                ret.append(valparts)
        return ret

    def __make_placeholders(self, vals):
        """make the placeholder values ("%s",%d etc) for a write command"""
        def make_placeholder(val):
            t = type(val)
            if t == types.IntType:
                return '%d'
            else:
                assert(t == types.StringType or t == types.UnicodeType,
                       'unexpected type %s' % str(t))
                return '"%s"'

        return ','.join(map(make_placeholder, vals))



class PhoneEntry:
    """(abstract) base class representing an event/contact entry
    
    data is kept roughly as it is stored in the phone
    """
    def __init__(self, data, format):
        raise NotImplementedError

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

    def to_xml(self):
        """return the opensync XML representation of this entry"""
        raise NotImplementedError

    def write(self, comms):
        """given an instance of PhoneComms, write this entry to the phone"""
        raise NotImplementedError

    def hash_data(self):
        """return a list of entry data in a predictable format, for hashing"""
        raise NotImplementedError

    class UnsupportedDataError(Exception):
        """Exception raised by __init__ when the data cannot be stored."""
        def __init__(self, msg):
            self.msg = msg
        def __str__(self):
            return self.msg


class PhoneEvent(PhoneEntry):
    """Class representing the events roughly as stored in the phone.

    class members:
       pos:         integer position/slot in phone memory
       name:        name/summary of event
       eventdt:     datetime or date object (if no time is set) for event
       duration:    timedelta object for duration of event
       alarmdt:     datetime object or None, for alarm time
       repeat_type: integer 0-5 for repeating events
       exceptions:  list of occurrences (0-based) which do not actually happen
    """
    def __init__(self, data, format):
        if format == "moto-event":
            self.__from_moto(data)
        elif format == "xml-event-doc":
            self.__from_xml(data)
        else:
            raise OpenSyncError("unhandled data format %s" % format,
                                opensync.ERROR_NOT_SUPPORTED)

    def get_objtype(self):
        return "event"

    def generate_uid(self):
        return str(self.pos)

    @staticmethod
    def unpack_uid(uid):
        """Unpack the string returned by generate_uid to a list of positions."""
        return [int(uid)]

    def num_pos(self):
        return 1

    def get_pos(self):
        return [self.pos]

    def set_pos(self, positions):
        assert(len(positions) == 1)
        self.pos = positions[0]

    def write(self, comms):
        comms.write_event(self.__to_moto())

    def hash_data(self):
        return self.__to_moto()

    def __from_moto(self, data):
        """grab stuff out of the list of values from the phone"""
        assert(type(data) == list and len(data) == 11)
        self.pos = data[0]
        self.name = data[1]
        timeflag = data[2]
        alarmflag = data[3]
        time = data[4]
        date = data[5]
        self.duration = timedelta(0, 0, 0, 0, data[6])
        alarmtime = data[7]
        alarmdate = data[8]
        self.repeat_type = data[9]
        self.exceptions = data[10]
        self.exceptions.sort() # just in case

        if timeflag:
            self.eventdt = parse_moto_time(date, time)
        else:
            self.eventdt = parse_moto_time(date)

        if alarmflag:
            self.alarmdt = parse_moto_time(alarmdate, alarmtime)
        else:
            self.alarmdt = None

    def __from_xml(self, data):
        """Parse XML event data.
        
        This function raises UnsupportedDataError for:
         * events that recur but can't be represented on the phone
         * events lasting past the end of a day (FIXME: split these up)
         * events before year 2000 (my phone doesn't allow them)
        """
        doc = xml.dom.minidom.parseString(data)
        event = doc.getElementsByTagName('Event')[0]

        # utility function for the XML processing below
        def getField(tagname, subtag='Content'):
            return getXMLField(event, tagname, subtag)

        self.pos = None
        self.name = getField('Summary')

        self.eventdt = parse_ical_time(getField('DateStarted'))
        durationstr = getField('Duration')
        if durationstr != '':
            self.duration = parse_ical_duration(durationstr)
            enddt = self.eventdt + self.duration
        else:
            enddt = parse_ical_time(getField('DateEnd'))
            self.duration = enddt - self.eventdt

        def get_date(dt):
            """Return the date that a datetime or date object falls on."""
            if isinstance(dt, datetime):
                return dt.date()
            else:
                assert(isinstance(dt, date))
                return dt

        # check if event crosses a day or is too old
        if self.eventdt.year < 2000:
            raise PhoneEvent.UnsupportedDataError('Event is too old')
        if get_date(self.eventdt) != get_date(enddt):
            raise PhoneEvent.UnsupportedDataError('Event crosses a day')

        triggerstr = getField('Alarm', 'AlarmTrigger')
        if triggerstr == '':
            self.alarmdt = None
        else:
            if triggerstr.startswith('-P') or triggerstr.startswith('P'):
                offset = parse_ical_duration(triggerstr)
                self.alarmdt = self.eventdt + offset
            else:
                self.alarmdt = parse_ical_time(triggerstr)

        rrules = event.getElementsByTagName('RecurrenceRule')
        exdates = event.getElementsByTagName('ExclusionDate')
        exrules = event.getElementsByTagName('ExclusionRule')
        (self.repeat_type, self.exceptions) = convert_rrule(rrules, exdates,
                                                          exrules, self.eventdt)

        if len(rrules) > 0 and self.repeat_type == MOTO_REPEAT_NONE:
            raise PhoneEvent.UnsupportedDataError('Unhandled recursion rule')


    def __to_moto(self):
        """generate motorola event-data list"""
        if isinstance(self.eventdt, datetime):
            timeflag = 1
            datestr = format_time(self.eventdt, PHONE_DATE)
            timestr = format_time(self.eventdt, PHONE_TIME)
        else:
            timeflag = 0
            datestr = self.eventdt.strftime(PHONE_DATE)
            timestr = '00:00'
        if self.alarmdt:
            alarmflag = 1
            alarmdatestr = format_time(self.alarmdt, PHONE_DATE)
            alarmtimestr = format_time(self.alarmdt, PHONE_TIME)
        else:
            alarmflag = 0
            alarmdatestr = '00-00-2000'
            alarmtimestr = '00:00'

        duration = int(self.duration.days) * 24 * 60
        duration += int(self.duration.seconds) / 60
        return (self.pos, self.name, timeflag, alarmflag, timestr, datestr,
                duration, alarmtimestr, alarmdatestr, self.repeat_type,
                self.exceptions)

    def to_xml(self):
        impl = xml.dom.minidom.getDOMImplementation()
        doc = impl.createDocument(None, 'vcal', None)
        top = doc.createElement('Event')
        doc.documentElement.appendChild(top)

        e = doc.createElement('Summary')
        appendXMLChild(doc, e, 'Content', self.name)
        top.appendChild(e)

        e = doc.createElement('DateStarted')
        if isinstance(self.eventdt, datetime):
            dtstart = self.eventdt.strftime(VCAL_DATETIME)
        else:
            dtstart = self.eventdt.strftime(VCAL_DATE)
            appendXMLChild(doc, e, 'Value', 'DATE')
        appendXMLChild(doc, e, 'Content', dtstart)
        top.appendChild(e)

        e = doc.createElement('DateEnd')
        endtime = self.eventdt + self.duration
        if isinstance(self.eventdt, datetime):
            dtend = endtime.strftime(VCAL_DATETIME)
        else:
            dtend = endtime.strftime(VCAL_DATE)
            appendXMLChild(doc, e, 'Value', 'DATE')
        appendXMLChild(doc, e, 'Content', dtend)
        top.appendChild(e)

        if self.alarmdt:
            alarm = doc.createElement('Alarm')
            appendXMLChild(doc, alarm, 'AlarmAction', 'DISPLAY')
            appendXMLChild(doc, alarm, 'AlarmDescription', self.name)
            alarmtime = self.alarmdt.strftime(VCAL_DATETIME)
            appendXMLChild(doc, alarm, 'AlarmTrigger', alarmtime)
            top.appendChild(alarm)

        e = doc.createElement('RecurrenceRule')
        if self.repeat_type == MOTO_REPEAT_DAILY:
            appendXMLChild(doc, e, 'Rule', 'FREQ=DAILY')
        elif self.repeat_type == MOTO_REPEAT_WEEKLY:
            appendXMLChild(doc, e, 'Rule', 'FREQ=WEEKLY')
        elif self.repeat_type == MOTO_REPEAT_MONTHLY_DATE:
            appendXMLChild(doc, e, 'Rule', 'FREQ=MONTHLY')
            appendXMLChild(doc, e, 'Rule', 'BYMONTHDAY=%d' % self.eventdt.day)
        elif self.repeat_type == MOTO_REPEAT_MONTHLY_DAY:
            appendXMLChild(doc, e, 'Rule', 'FREQ=MONTHLY')
            day = VCAL_DAYS[self.eventdt.weekday()]
            # compute the week number that the event falls in
            if self.eventdt.day % 7 == 0:
                weeknum = self.eventdt.day / 7
            else:
                weeknum = self.eventdt.day / 7 + 1
            appendXMLChild(doc, e, 'Rule', 'BYDAY=%d%s' % (weeknum, day))
        elif self.repeat_type == MOTO_REPEAT_YEARLY:
            appendXMLChild(doc, e, 'Rule', 'FREQ=YEARLY')
            appendXMLChild(doc, e, 'Rule', 'BYMONTH=%d' % self.eventdt.month)
            appendXMLChild(doc, e, 'Rule', 'BYMONTHDAY=%d' % self.eventdt.day)

        if e.hasChildNodes():
            top.appendChild(e)

        if self.exceptions != []:
            assert(self.repeat_type != MOTO_REPEAT_NONE)
            
            # create an rrule object for this recurrence
            if self.repeat_type == MOTO_REPEAT_MONTHLY_DATE:
                rrule = dateutil.rrule.rrule(dateutil.rrule.MONTHLY,
                                             bymonthday=self.eventdt.day,
                                             dtstart=self.eventdt)
            elif self.repeat_type == MOTO_REPEAT_MONTHLY_DAY:
                weekday = dateutil.rrule.weekdays[self.eventdt.weekday()]
                # weeknum is calculated above in the XML generation
                rrule = dateutil.rrule.rrule(dateutil.rrule.MONTHLY,
                                             byweekday=weekday(+weeknum),
                                             dtstart=self.eventdt)
            else:
                if self.repeat_type == MOTO_REPEAT_DAILY:
                    freq = dateutil.rrule.DAILY
                elif self.repeat_type == MOTO_REPEAT_WEEKLY:
                    freq = dateutil.rrule.WEEKLY
                elif self.repeat_type == MOTO_REPEAT_YEARLY:
                    freq = dateutil.rrule.YEARLY
                rrule = dateutil.rrule.rrule(freq, dtstart=self.eventdt)

            # work out which dates the exceptions correspond to and
            # generate ExceptionDate nodes for them
            for exnum in self.exceptions:
                e = doc.createElement('ExclusionDate')
                appendXMLChild(doc, e, 'Content',
                               format_time(rrule[exnum], VCAL_DATE))
                appendXMLChild(doc, e, 'Value', 'DATE')
                top.appendChild(e)

        return doc.toxml()

class PhoneContactBase(PhoneEntry):
    """Class representing the contacts roughly as stored in the phone.

    PhoneContactBase represents the common fields of a logical identity that
    may be split across several entries in the phone book for different contact
    types.

    class members:
       children:        list of PhoneContact objects, that contain attributes
                        unique to each entry
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
    def __init__(self, child, data, format, revcategories):
        """This init method should ONLY be called from within PhoneContact."""
        self.children = [child]
        if format == "moto-contact":
            self.__from_moto(data)
        elif format == "xml-contact-doc":
            self.__from_xml(data, revcategories)
        else:
            raise OpenSyncError("unhandled data format %s" % format,
                                opensync.ERROR_NOT_SUPPORTED)

    def get_objtype(self):
        return "contact"

    def generate_uid(self):
        return ','.join(map(lambda c: str(c.pos), self.children))

    @staticmethod
    def unpack_uid(uid):
        """Unpack the string returned by generate_uid to a list of positions."""
        return map(int, uid.split(','))

    def num_pos(self):
        return len(self.children)

    def get_pos(self):
        return map(lambda c: c.pos, self.children)

    def set_pos(self, positions):
        # FIXME: if numbers/emails are added to an existing contact, the
        #        number of positions required as well as the UID might change
        assert(len(positions) == len(self.children))
        for (p, c) in zip(positions, self.children):
            c.pos = p

    def write(self, comms):
        for child in self.children:
            comms.write_contact(child._to_moto())

    def hash_data(self):
        ret = []
        for child in self.children:
            ret.append(list(child._to_moto()))
        return ret

    def __from_moto(self, data):
        """grab stuff out of the list of values from the phone"""
        assert(type(data) == list and len(data) >= 24)
        self.name = data[3]
        self.categorynum = data[9]
        self.firstlast_enabled = data[11]
        self.firstlast_index = data[12]
        self.nickname = data[22]
        if data[23] == '':
            self.birthday = None
        else:
            self.birthday = parse_moto_time(data[23])

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

    def __from_xml(self, xmldata, revcategories):
        doc = xml.dom.minidom.parseString(xmldata)

        # utility function for the XML processing below
        def getField(tagname, subtag='Content'):
            return getXMLField(doc, tagname, subtag)

        # set defaults that will be overwritten by the XML data if possible
        self.firstlast_enabled = MOTO_INVALID
        self.firstlast_index = 0

        # handle the name and formatted name fields
        self.name = getField('FormattedName')
        if self.name != '':
            # FIXME: cheesy attempt at taking apart the FormattedName
            last = getField('Name', 'LastName')
            if last != '':
                lastidx = self.name.find(last)
                if lastidx == 0:
                    first = getField('Name', 'FirstName')
                    if first != '':
                        firstidx = self.name.find(first)
                        if firstidx != -1:
                            self.firstlast_enabled = 1
                            self.firstlast_index = firstidx
                elif lastidx != -1:
                    self.firstlast_enabled = 0
                    self.firstlast_index = lastidx
        else:
            # compute FormattedName from Name
            nameparts = map(lambda p: getField('Name', p), XML_NAME_PARTS)
            nameparts = filter(lambda x: x != '', nameparts)
            self.name = ' '.join(nameparts)
            self.firstlast_enabled = 0
            self.firstlast_index = self.name.index(getField('Name', 'LastName'))

        catname = getField('Categories', 'Category').lower()
        self.categorynum = revcategories.get(catname, MOTO_CATEGORY_DEFAULT)
        self.nickname = getField('Nickname')
        bdaystr = getField('Birthday')
        if bdaystr != '':
            self.birthday = parse_ical_time(bdaystr)
        else:
            self.birthday = None

        # NOW WE PROCESS ALL THE CONTACT INFO AND CREATE CHILDREN

        # we always have one (uninitialised) child to start with, use it first
        assert(len(self.children) == 1)
        self.firstchild_used = False

        # utility function for creating children
        def makeChild(contact, contacttype, is_pref=False, address=None):
            args = (self, contact, contacttype, int(is_pref), address)
            if self.firstchild_used:
                child = PhoneContact(args, "XXX-parentcontact-hack")
                self.children.append(child)
            else:
                self.children[0]._from_parent(args)
                self.firstchild_used = True

        # process telephone numbers and email addresses
        # create a list [(contacttype, contact, is_pref)]
        contacts = []
        for elt in getElementsByTagNames(doc, set(['Telephone', 'EMail']), []):
            content = getXMLField(elt, 'Content')
            if elt.tagName == 'Telephone':
                # filter out any illegal characters from the phone number
                content = filter(lambda c: c in TEL_NUM_DIGITS, content)
                types = map(lambda e: getXMLText(e).lower(),
                            elt.getElementsByTagName('Type'))
                is_pref = 'pref' in types
                moto_type = MOTO_CONTACT_DEFAULT
                for t in types:
                    if VCARD_CONTACT_TYPES.has_key(t):
                        moto_type = VCARD_CONTACT_TYPES[t]
                        break
            else: # email
                is_pref = False
                moto_type = MOTO_CONTACT_EMAIL
            contacts.append((moto_type, content, is_pref))

        # process addresses, create a hash from contacttype to address
        # FIXME: addresses that don't map to moto contact types are dropped
        addresses = {}
        for adr in doc.getElementsByTagName('Address'):
            address = map(lambda p: getXMLField(adr, p), XML_ADDRESS_PARTS)
            types = map(lambda e: getXMLText(e).lower(),
                        adr.getElementsByTagName('Type'))
            for t in types:
                if VCARD_ADDRESS_TYPES.has_key(t):
                    moto_type = VCARD_ADDRESS_TYPES[t]
                    addresses[moto_type] = address
                    break

        # create a child for each telephone/address pair or email
        for (moto_type, contact, is_pref) in contacts:
            if moto_type == MOTO_CONTACT_EMAIL:
                addr = None
            else:
                addr = addresses.get(moto_type)
            makeChild(contact, moto_type, is_pref, addr)

        if not self.firstchild_used:
            # this entry has no contact data
            raise PhoneEvent.UnsupportedDataError('No telephone or email data')

    def to_xml(self, categories):
        impl = xml.dom.minidom.getDOMImplementation()
        doc = impl.createDocument(None, 'contact', None)
        top = doc.documentElement

        e = doc.createElement('FormattedName')
        appendXMLChild(doc, e, 'Content', self.name)
        top.appendChild(e)

        e = doc.createElement('Name')
        if self.firstlast_enabled == MOTO_INVALID:
            # FIXME: have to guess at name split, this is what opensync does:
            appendXMLChild(doc, e, 'LastName', self.name)
        else:
            first = self.name[:self.firstlast_index].strip()
            last = self.name[self.firstlast_index:].strip()
            if self.firstlast_enabled:
                appendXMLChild(doc, e, 'FirstName', last)
                appendXMLChild(doc, e, 'LastName', first)
            else:
                appendXMLChild(doc, e, 'FirstName', first)
                appendXMLChild(doc, e, 'LastName', last)
        top.appendChild(e)

        if self.nickname != '':
            e = doc.createElement('Nickname')
            appendXMLChild(doc, e, 'Content', self.nickname)
            top.appendChild(e)

        if self.birthday:
            bdaystr = format_time(self.birthday, VCAL_DATE)
            e = doc.createElement('Birthday')
            appendXMLChild(doc, e, 'Content', bdaystr)
            top.appendChild(e)

        e = doc.createElement('Categories')
        appendXMLChild(doc, e, 'Category', categories[self.categorynum])
        top.appendChild(e)

        for child in self.children:
            for node in child._child_xml(doc):
                top.appendChild(node)

        return doc.toxml()


class PhoneContact:
    """Class representing the contacts roughly as stored in the phone.

    Data common to multiple entries is stored in PhoneContactBase.

    class members:
       parent:          parent PhoneContactBase object
       pos:             position of this entry in the phone book
       contact:         contact data (number or email address)
       contacttype:     integer representing contact type (home/work/etc, email)
       numtype:         one of MOTO_NUMTYPE_*
       voicetag:        flag if voice-dial tag is set for this entry
       ringerid:        ringer ID for this entry
       primaryflag:    flag set on the "primary" contact for a given identity
       profile_icon:    profile icon number
       picture_path:    path to picture for this entry (on the phone's VFS)
       address:         None, or list of address parts
    """
    def __init__(self, data, format, revcategories=None):
        if format == "XXX-parentcontact-hack":
            self._from_parent(data)
        else:
            if format == "moto-contact":
                self.__from_moto(data)
            parent = PhoneContactBase(self, data, format, revcategories)
            self.parent = parent

    def __getattr__(self, name):
        """If we don't have an attribute, return the parent's version that
        is common to all of its children.
        """
        assert(hasattr(self, 'parent'))
        return getattr(self.parent, name)

    def _to_moto(self):
        """Generate motorola contact-data list."""
        if self.birthday:
            birthdaystr = format_time(self.birthday, PHONE_DATE)
        else:
            birthdaystr = ''
        if self.address:
            (street1, street2, city, state, postcode, country) = self.address
        else:
            street1 = street2 = city = state = postcode = country = ''
        return (self.pos, self.contact, self.numtype, self.name,
                self.contacttype, self.voicetag, self.ringerid, 0,
                self.primaryflag, self.categorynum, self.profile_icon,
                self.firstlast_enabled, self.firstlast_index, self.picture_path,
                0, 0, street2, street1, city, state, postcode, country,
                self.nickname, birthdaystr)

    def __from_moto(self, data):
        """grab stuff out of the list of values from the phone"""
        assert(type(data) == list and len(data) >= 24)
        self.pos = data[0]
        self.contact = data[1]
        self.numtype = data[2]
        self.contacttype = data[4]
        self.voicetag = data[5]
        self.ringerid = data[6]
        assert(data[7] == 0) # backlight flag?
        self.primaryflag = data[8]
        self.profile_icon = data[10]
        self.picture_path = data[13]
        assert(data[14] == 0) # unknown?
        assert(data[15] == 0) # unknown?
        self.address = (data[17], data[16], data[18],
                        data[19], data[20], data[21])
        if len(data) >= 25:
            assert(data[24] == '') # unknown? old phones don't have it

    def _child_xml(self, doc):
        ret = []

        if self.contacttype == MOTO_CONTACT_EMAIL:
            e = doc.createElement('EMail')
        elif self.contacttype == MOTO_CONTACT_MAILINGLIST:
            # the 'contact' is a space-separated list of other contact positions
            assert(False, "mailing lists aren't handled yet, sorry") # FIXME
        else:
            e = doc.createElement('Telephone')
            appendXMLChild(doc, e, 'Type',
                           MOTO_CONTACT_TYPES[self.contacttype].upper())
            if self.primaryflag:
                appendXMLChild(doc, e, 'Type', 'PREF')
        appendXMLChild(doc, e, 'Content', self.contact)
        ret.append(e)

        if self.address:
            e = doc.createElement('Address')
            for (part, val) in zip(XML_ADDRESS_PARTS, self.address):
                appendXMLChild(doc, e, part, val)
            if e.hasChildNodes():
                if MOTO_ADDRESS_TYPES.has_key(self.contacttype):
                    appendXMLChild(doc, e, 'Type',
                                   MOTO_ADDRESS_TYPES[self.contacttype].upper())
                ret.append(e)

        return ret

    def _from_parent(self, data):
        """Initialise a child, should only be called by its parent."""
        (parent, contact, contacttype, primary, address) = data
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


class PhoneAccess:
    """Grab-bag class of utility functions.
     
     Interfaces between PhoneComms/PhoneEntry classes and SyncClass below.
     """
    def __init__(self, comms):
        self.comms = comms
        self.sn = comms.read_serial()
        self.positions_used = {}
        for objtype in SUPPORTED_OBJTYPES:
            self.positions_used[objtype] = set([])

        # check that the phone supports the features we need
        features = comms.read_features()
        for (bit, desc) in REQUIRED_FEATURES:
            if not features[bit]:
                raise OpenSyncError(desc + ' feature not present',
                                    opensync.ERROR_NOT_SUPPORTED)

        # read current time on the phone, check if it matches our time
        # if not, print a warning about timezones
        # FIXME: use these times to guess the phone's UTC offset and timezone
        timestr = comms.read_time()[:-3]
        phone_now = time.strptime(timestr, '%y/%m/%d,%H:%M:%S')
        local_now = time.localtime()
        if abs(time.mktime(phone_now) - time.mktime(local_now)) > 60 * 30:
            print "WARNING: Phone appears to be in a different timezone!"
            print "Phone time is " + time.strftime('%d/%m/%Y %H:%M', phone_now)
            # FIXME: maybe we should refuse to continue?

    def list_changes(self, objtype):
        """Return a list of change objects for all entries of the given type."""

        if objtype == 'contact':
            self.__init_categories()

            # XXX FIXME, nasty hack:
            # fill in positions_used so that none < minpos are allocated
            # XXX FIXME, even nastier hack:
            # also make sure positions < 10 aren't allocated by us, as these
            # are used for the quick-dial feature the phone
            (minpos, maxpos, _, _) = self.comms.read_contact_params()
            self.positions_used['contact'] = set(range(0, max(minpos, 10)))

            # sort contacts by name, and attempt to merge adjacent ones
            entries = map(lambda d: PhoneContact(d, 'moto-contact',
                                                 self.revcategories),
                          self.comms.read_contacts())
            entries.sort(key=lambda c: (c.name, c.pos))
            i = 0
            while i < (len(entries) - 1):
                if entries[i].parent.merge(entries[i + 1].parent):
                    del entries[i + 1]
                else:
                    i += 1
        elif objtype == 'event':
            entries = map(lambda d: PhoneEvent(d, 'moto-event'),
                          self.comms.read_events())
        else:
            assert(False, 'Unknown objtype %s' % objtype)

        ret = []
        for entry in entries:
            change = opensync.OSyncChange()
            change.objtype = objtype
            change.uid = self.__generate_uid(entry)
            change.format = "xml-%s-doc" % objtype
            if objtype == 'contact':
                change.data = entry.to_xml(self.categories)
            else:
                change.data = entry.to_xml()
            change.hash = self.__gen_hash(entry)
            ret.append(change)
            self.positions_used[objtype].update(set(entry.get_pos()))
        return ret

    def delete_entry(self, uid):
        """Delete an event with the given UID
        
        Returns True on success, False otherwise.
        """
        objtype, positions = self.__uid_to_pos(uid)
        for pos in positions:
            if objtype == 'event':
                self.comms.delete_event(pos)
            elif objtype == 'contact':
                self.comms.delete_contact(pos)
            self.positions_used[objtype].remove(pos)
        return True

    def update_entry(self, change):
        """Update an entry or add a new one, from the OSyncChange object.
        
        Returns True on success, False otherwise.
        """
        try:
            if change.objtype == 'event':
                entry = PhoneEvent(change.data, change.format)
            elif change.objtype == 'contact':
                entry = PhoneContact(change.data, change.format,
                                     self.revcategories)
        except PhoneEntry.UnsupportedDataError, e:
            print ("Warning: %s is unsupported by moto-sync (%s), ignored"
                   % (change.uid, str(e)))
            # we have an entry that can't be stored on the phone
            # if its modified and we've seen it before, delete it
            # otherwise just ignore it
            if (change.changetype == opensync.CHANGE_MODIFIED
                and PhoneAccess.uid_seen(change.uid)):
                change.changetype = opensync.CHANGE_DELETED
                change.data = None
                return self.delete_entry(change.uid)
            else:
                return False
        
        if change.changetype == opensync.CHANGE_ADDED:
            self.__get_free_positions(entry)
            change.uid = self.__generate_uid(entry)
        else:
            _, positions = self.__uid_to_pos(change.uid)
            entry.set_pos(positions)
        
        change.hash = self.__gen_hash(entry)
        entry.write(self.comms)
        
        return True

    def __init_categories(self):
        """Initialise a hash and reverse hash of category IDs."""
        self.categories = {}
        self.revcategories = {}

        # for each category we get: pos,name,other junk
        for data in self.comms.read_categories():
            catid, catname = data[:2]
            self.categories[catid] = catname
            self.revcategories[catname.lower()] = catid

    def __gen_hash(self, entry):
        """generate hash for the opensync hashtable, md5 of all the data"""
        m = md5.new()
        for item in entry.hash_data():
            m.update(str(item))
        return m.hexdigest()

    def __generate_uid(self, entry):
        """Generate a "hopefully unique" UID for an entry.

        Uses the last 8 digit's of the phone's IMEI to do so.
        """
        return ("moto-%s-%s@%s"
                % (entry.get_objtype(), entry.generate_uid(), self.sn[-8:]))

    @staticmethod
    def uid_seen(uid):
        """Returns true iff the given UID is one of ours.
        
        Can't check the serial number because this is static, so we just
        check if it's a moto-sync UID and hope that's good enough.
        """
        return uid.startswith("moto-")

    def __uid_to_pos(self, uid):
        """Reverse the generate_uid function above.
        
        Also checks that it is one of ours.
        """
        moto, objtype, lastpart = uid.split('-', 2)
        assert(moto == "moto" and objtype in SUPPORTED_OBJTYPES,
               'Invalid UID: %s' % uid)
        lastpos = lastpart.rindex('@')
        assert(lastpart[lastpos + 1:] == self.sn[-8:],
               'Entry not created on this phone')
        
        if objtype == "event":
            positions = PhoneEvent.unpack_uid(lastpart[:lastpos])
        elif objtype == "contact":
            positions = PhoneContactBase.unpack_uid(lastpart[:lastpos])
        return objtype, positions

    # FIXME: check for minimum/maximum number of events in the phone
    def __get_free_positions(self, entry):
        """Allocate free positions for a new entry."""
        i = 0
        newposns = []
        used = self.positions_used[entry.get_objtype()]
        num_pos = entry.num_pos()
        while len(newposns) < num_pos:
            while i < len(used) and i in used:
                i += 1
            used.add(i)
            newposns.append(i)
        entry.set_pos(newposns)


def stdexceptions(func):
    """Decorator used to wrap every method in SyncClass with the same
    exception handlers. Reports any exception back to opensync as an error.
    """
    def new_func(*args, **kwds):
        context = args[1] # context is always the first argument after 'self'
        try:
            func(*args, **kwds)
            context.report_success()
        except OpenSyncError, e:
            e.report(context)
        except (IOError, OSError), e:
            context.report_error(opensync.ERROR_IO_ERROR, str(e))
        except:
            context.report_error(opensync.ERROR_GENERIC, traceback.format_exc())
    new_func.func_name = func.func_name
    return new_func


class SyncClass:
    """Synchronisation class used by OpenSync."""

    def __init__(self, member):
        self.member = member

    @stdexceptions
    def connect(self, ctx):
        self.config = self.__parse_config(self.member.config)
        self.hashtable = opensync.OSyncHashTable()
        if not self.hashtable.load(self.member):
            raise OpenSyncError('hashtable load failed',
                                opensync.ERROR_INITIALIZATION)

        self.comms = PhoneComms(self.config.device)
        self.access = PhoneAccess(self.comms)

    @stdexceptions
    def get_changeinfo(self, ctx):
        for objtype in SUPPORTED_OBJTYPES:
            if self.member.objtype_enabled(objtype):
                if self.member.get_slow_sync(objtype):
                    self.hashtable.set_slow_sync(objtype)

                for change in self.access.list_changes(objtype):
                    self.hashtable.detect_change(change)
                    if change.changetype != opensync.CHANGE_UNMODIFIED:
                        change.report(ctx)
                        self.hashtable.update_hash(change)

                self.hashtable.report_deleted(ctx, objtype)

    @stdexceptions
    def commit_change(self, ctx, change):
        if change.objtype not in SUPPORTED_OBJTYPES:
            raise OpenSyncError('unsupported objtype %s' % change.objtype,
                                opensync.ERROR_NOT_SUPPORTED)
        if change.changetype == opensync.CHANGE_DELETED:
            success = (PhoneAccess.uid_seen(change.uid)
                       and self.access.delete_entry(change.uid))
        else:
            success = self.access.update_entry(change)
        if success:
            self.hashtable.update_hash(change)

    @stdexceptions
    def sync_done(self, ctx):
        self.hashtable.forget()

    @stdexceptions
    def disconnect(self, ctx):
        del self.access
        del self.comms
        self.hashtable.close()
        del self.hashtable

    def finalize(self):
        del self.member

    def __parse_config(self, configstr):
        try:
            doc = xml.dom.minidom.parseString(configstr)
        except:
            raise OpenSyncError('failed to parse config data',
                                opensync.ERROR_MISCONFIGURATION)

        class Config:
            pass
        ret = Config()
        ret.device = getXMLField(doc, 'device')
        if ret.device == '':
            raise OpenSyncError('device not specified in config file',
                                opensync.ERROR_MISCONFIGURATION)

        return ret


def initialize(member):
    return SyncClass(member)


def get_info(info):
    info.name = "moto-sync"
    for objtype in SUPPORTED_OBJTYPES:
        info.accept_objtype(objtype)
        info.accept_objformat(objtype, "xml-%s-doc" % objtype)
