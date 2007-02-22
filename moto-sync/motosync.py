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

import sys, os, types, md5, time, calendar, re
import xml.dom.minidom
from datetime import date, datetime, timedelta, time as datetime_time
import dateutil.parser, dateutil.rrule, dateutil.tz
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

# debug/test options:
DEBUG_OUTPUT = False # if enabled, logs all interaction with the phone to stdout
WARNING_OUTPUT = True # if enabled, prints warnings to stderr (reccommended)
WRITE_ENABLED = True # if disabled, prevents changes from being made to phone

# Regular expression for a Bluetooth MAC address
BT_MAC_RE = re.compile(r'^([0-9a-f]{2}:){5}[0-9a-f]{2}$', re.IGNORECASE)

# name of the SDP service to use with Bluetooth
BT_SERVICE_NAME = 'Dial-up networking Gateway'
# name of the channel to try if we don't find the above service
BT_DEFAULT_CHANNEL = 1

# object types supported by this plugin
SUPPORTED_OBJTYPES = ['event', 'contact']

CAPABILITIES = """<?xml version="1.0"?>
<capabilities>
    <contact>
        <Address />
        <Birthday />
        <Categories />
        <EMail />
        <FormattedName />
        <Name />
        <Nickname />
        <Telephone />
    </contact>
    <event>
        <Summary />
        <DateStarted />
        <DateEnd />
        <Duration />
        <Alarm />
        <RecurrenceRule />
        <ExceptionDateTime />
    </event>
</capabilities>
"""

# my phone doesn't like it if you read more than this many entries at a time
ENTRIES_PER_READ = 15

# time and date formats
PHONE_TIME = '%H:%M'
PHONE_DATE = '%m-%d-%Y' # yuck!
VCAL_DATETIME = '%Y%m%dT%H%M%SZ'
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

# mapping from phone's contact type to XML number types
MOTO_PHONE_CONTACT_TYPES = {
    2: 'Voice',
    3: 'Cellular',
    4: 'Fax',
    5: 'Pager',
    11: 'Voice' # shows up as "other", seen on a RAZR V3x
}

# as above, but to TelephoneLocation
MOTO_PHONE_CONTACT_LOCATIONS = {
    0: 'Work',
    1: 'Home'
}

# reverse of the above (almost): mapping from vcard to phone's contact type
VCARD_CONTACT_TYPES = {
    'Work':     0,
    'Home':     1,
    'Voice':    2,
    'Cellular': 3,
    'Car':      3,
    'Message':  5,
    'Fax':      4,
    'Pager':    5,
}

# (dodgy) mapping from address types to motorola contact types
# can also have dom/intl/postal/parcel... these don't really make sense here
VCARD_ADDRESS_TYPES = {
    'Work':  0,
    'Home':  1,
}

# reverse of the above
MOTO_ADDRESS_TYPES = {
    0: 'Work',
    1: 'Home',
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

# number of quick-dial entries in the phonebook; we avoid allocating these
MOTO_QUICKDIAL_ENTRIES = 10

# logical order of the fields in structured XML data
XML_NAME_PARTS = 'Prefix FirstName Additional LastName Suffix'.split()
XML_ADDRESS_PARTS = ('Street ExtendedAddress'
                     + ' Locality Region PostalCode Country').split()

# legal characters in telephone numbers
TEL_NUM_DIGITS = set('+0123456789')

# how far into the future to process exceptions for recurring events
RRULE_FUTURE = timedelta(365) # 1 year


def debug(msg):
    """Print a debug message, if enabled."""
    if DEBUG_OUTPUT:
        sys.stdout.write("moto-sync: " + msg + "\n")

def warning(msg):
    """Print a warning message, if enabled."""
    if WARNING_OUTPUT:
        sys.stderr.write("moto-sync: Warning: " + msg + "\n")

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

def getXMLText(el):
    """Returns all text within a given XML node."""
    return ''.join([n.data for n in el.childNodes if n.nodeType == n.TEXT_NODE])

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
    
    # XXX FIXME: whole function needs reworkign for new XML format
    return (MOTO_REPEAT_NONE, [])
    
    # can't support multiple rules
    if len(rulenodes) != 1:
        return (MOTO_REPEAT_NONE, [])
    rulenode = rulenodes[0]

    # build hash of rule parts
    rules = {}
    for node in rulenode.childNodes:
        key = node.nodeName.lower()
        val = getXMLText(node).lower()
        if key[:1] == 'by':
            val = set(val.split(','))
        rules[key] = val

    # extract the parts
    assert(rules.has_key('content')) # required
    freq = rules['content'].lower()
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
    if (freq == 'daily' and (not byday or byday == byalldays)
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
    # XXX FIXME: totally different rrule format
    # rulestr = ';'.join(map(getXMLText, rulenode.getElementsByTagName('Rule')))
    # ruleset = dateutil.rrule.rrulestr(rulestr, dtstart=eventdt, forceset=True)

    # are there any future occurrences of this event?
    now = datetime.now()
    if ruleset.after(now) == None:
        return (MOTO_REPEAT_NONE, [])

    # what events would happen if there were no exceptions?
    all_occurrences = ruleset.between(now, now + RRULE_FUTURE)

    # add in the exception dates and rules (if any)
    for node in exdates:
        for e in getElementsByTagNames(node, 'Content'):
            ruleset.exdate(dateutil.parser.parse(getXMLText(e)))

    # XXX FIXME: totally different rrule format
    #for node in exrules:
    #    ruleset.exrule(dateutil.rrule.rrulestr(getXMLField(node, 'Content')))

    # are there any future occurrences of this event if we consider exceptions?
    if ruleset.after(datetime.now()) == None:
        return (MOTO_REPEAT_NONE, [])

    # what events will happen with exceptions
    excepted_occurrences = frozenset(ruleset.between(now, now + RRULE_FUTURE))

    # work out which events don't happen
    exceptions = []
    for num in range(all_occurrences):
        if all_occurrences[num] not in excepted_occurrences:
            exceptions.append(num)

    return (moto_type, exceptions)


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
        self.__fd = self.__btsock = None
        self.max_events = None
        self.num_events = None
        self.event_name_len = None
        self.event_max_exceptions = None
        self.min_contact_pos = None
        self.max_contact_pos = None
        self.contact_data_len = None
        self.contact_name_len = None

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
            found = bluetooth.find_service(name=BT_SERVICE_NAME,
                                           address=self.devstr)
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
                warning('tty module not present, unable to set raw mode')

        # reset the phone and send it a bunch of init strings
        self.__do_cmd('AT&F')      # reset to factory defaults
        self.__do_cmd('AT+MODE=0') # ?
        self.__do_cmd('ATE0Q0V1')  # echo off, result codes off, verbose results

        # use ISO 8859-1 encoding for data values, for easier debugging
        # FIXME: change to UCS2?
        self.__do_cmd('AT+CSCS="8859-1"')

        (maxevs, numevs, namelen, max_except, _) = self.read_event_params()
        self.max_events = maxevs
        self.num_events = numevs
        self.event_name_len = namelen
        self.event_max_exceptions = max_except

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
        if not self.__calendar_open:
            self.__do_cmd('AT+MDBL=1')
            self.__calendar_open = True

    def close_calendar(self):
        """Close the calendar."""
        if self.__calendar_open:
            self.__do_cmd('AT+MDBL=0')
            self.__calendar_open = False

    def read_event_params(self):
        """Read calendar/datebook parameters.
        
        Returns: maximum number of events,
                 number of events currently stored,
                 length of title/name field,
                 maximum number of event exceptions
                 maximum number of event exception types (?)
        """
        self.open_calendar()
        data = self.__do_cmd('AT+MDBR=?') # read event parameters
        return self.__parse_results('MDBR', data)[0]

    def read_events(self):
        """read the list of all events on the phone"""
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
                    raise opensync.Error('unexpected exception type %d'%extype)
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
        placeholders = self.__make_placeholders(data)
        self.__do_cmd(('AT+MPBW=' + placeholders) % tuple(data), WRITE_ENABLED)

    def delete_contact(self, pos):
        """delete the contact at a given position"""
        self.close_calendar()
        self.__do_cmd('AT+MPBW=%d' % pos, WRITE_ENABLED)

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
        debug('<-- ' + ret)
        if c == '': # EOF, shouldn't happen
            raise opensync.Error('Unexpected EOF talking to phone',
                                 opensync.ERROR_IO_ERROR)
        return ret

    def __do_cmd(self, cmd, reallydoit=True):
        """Send a command to the phone and wait for its response.

        If it succeeds, return lines as a list; otherwise raise an exception.
        """
        cmd = cmd.encode('iso_8859_1')
        debug('--> ' + cmd)
        ret = []
        if reallydoit:
            cmd = cmd + '\r'
            if self.__fd:
                os.write(self.__fd, cmd)
            elif self.__btsock:
                self.__btsock.send(cmd)
            line = self.__readline()
        else:
            line = 'OK'
        while line != 'OK' and line != 'ERROR':
            ret.append(line)
            line = self.__readline()
        if line == 'OK':
            return ret
        else:
            raise opensync.Error("Error in phone command '%s'" % cmd,
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
                        ranges = [map(int, s.split('-', 1)) for s in subparts]
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
        """Return string expandion placeholders ("%s",%d etc) for a write."""
        def make_placeholder(val):
            """Return a single placeholder based on the given value's type."""
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


class PhoneEvent(PhoneEntry):
    """Class representing the events roughly as stored in the phone.
    This class should not be instantiated directly, use one of PhoneEventMoto
    or PhoneEventXML depending on the data format.

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
        comms.write_event(self.__to_moto())

    def hash_data(self):
        """return a list of entry data in a predictable format, for hashing"""
        return self.__to_moto()

    def __truncate_fields(self, comms):
        """enforce length limits by truncating data"""
        self.name = self.name[:comms.event_name_len]
        self.exceptions = self.exceptions[:comms.event_max_exceptions]

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
        """Returns OpenSync XML representation of this event."""
        impl = xml.dom.minidom.getDOMImplementation()
        doc = impl.createDocument(None, 'event', None)
        top = doc.documentElement

        e = doc.createElement('Summary')
        appendXMLChild(doc, e, 'Content', self.name)
        top.appendChild(e)

        e = doc.createElement('DateStarted')
        if isinstance(self.eventdt, datetime):
            dtstart = self.eventdt.strftime(VCAL_DATETIME)
        else:
            dtstart = self.eventdt.strftime(VCAL_DATE)
            e.setAttribute('DateValue', 'DATE')
        appendXMLChild(doc, e, 'Content', dtstart)
        top.appendChild(e)

        e = doc.createElement('DateEnd')
        endtime = self.eventdt + self.duration
        if isinstance(self.eventdt, datetime):
            dtend = endtime.strftime(VCAL_DATETIME)
        else:
            dtend = endtime.strftime(VCAL_DATE)
            e.setAttribute('DateValue', 'DATE')
        appendXMLChild(doc, e, 'Content', dtend)
        top.appendChild(e)

        if self.alarmdt:
            alarm = doc.createElement('Alarm')
            appendXMLChild(doc, alarm, 'AlarmAction', 'DISPLAY')
            e = doc.createElement('AlarmDescription')
            appendXMLChild(doc, e, 'Content', self.name)
            alarm.appendChild(e)
            alarmtime = self.alarmdt.strftime(VCAL_DATETIME)
            appendXMLChild(doc, alarm, 'AlarmTrigger', alarmtime)
            top.appendChild(alarm)

        e = doc.createElement('RecurrenceRule')
        if self.repeat_type == MOTO_REPEAT_DAILY:
            appendXMLChild(doc, e, 'Content', 'DAILY')
        elif self.repeat_type == MOTO_REPEAT_WEEKLY:
            appendXMLChild(doc, e, 'Content', 'WEEKLY')
        elif self.repeat_type == MOTO_REPEAT_MONTHLY_DATE:
            appendXMLChild(doc, e, 'Content', 'MONTHLY')
            appendXMLChild(doc, e, 'ByMonthDay', str(self.eventdt.day))
        elif self.repeat_type == MOTO_REPEAT_MONTHLY_DAY:
            appendXMLChild(doc, e, 'Content', 'MONTHLY')
            day = VCAL_DAYS[self.eventdt.weekday()]
            # compute the week number that the event falls in
            if self.eventdt.day % 7 == 0:
                weeknum = self.eventdt.day / 7
            else:
                weeknum = self.eventdt.day / 7 + 1
            appendXMLChild(doc, e, 'ByDay', '%d%s' % (weeknum, day))
        elif self.repeat_type == MOTO_REPEAT_YEARLY:
            appendXMLChild(doc, e, 'Content', 'YEARLY')
            appendXMLChild(doc, e, 'ByMonth', str(self.eventdt.month))
            appendXMLChild(doc, e, 'ByMonthDay', str(self.eventdt.day))

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
            e = doc.createElement('ExceptionDateTime')
            e.setAttribute('DateValue', 'DATE')
            for exnum in self.exceptions:
                appendXMLChild(doc, e, 'Content',
                               format_time(rrule[exnum], VCAL_DATE))
            if e.hasChildNodes():
                top.appendChild(e)

        return doc.toxml()


class PhoneEventMoto(PhoneEvent):
    """Constructor for the PhoneEvent object with data in Motorola format"""
    def __init__(self, data):
        """grab stuff out of the list of values from the phone"""
        PhoneEvent.__init__(self)
        assert(type(data) == list and len(data) == 11)
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
        self.exceptions = data[10]
        self.exceptions.sort() # just in case

        if timeflag:
            self.eventdt = parse_moto_time(datestr, timestr)
        else:
            self.eventdt = parse_moto_time(datestr)

        if alarmflag:
            self.alarmdt = parse_moto_time(alarmdate, alarmtime)
        else:
            self.alarmdt = None


class PhoneEventXML(PhoneEvent):
    """Constructor for the PhoneEvent object with data in OpenSync XML format"""
    def __init__(self, data):
        """Parse XML event data.
        
        This function raises UnsupportedDataError for:
         * events that recur but can't be represented on the phone
         * events before year 2000 (my phone doesn't allow them)
        """
        PhoneEvent.__init__(self)
        doc = xml.dom.minidom.parseString(data)
        event = doc.getElementsByTagName('event')[0]

        def getField(tagname, subtag='Content'):
            """utility function for the XML processing below"""
            return getXMLField(event, tagname, subtag)

        self.pos = None
        self.name = getField('Summary')

        self.eventdt = parse_ical_time(getField('DateStarted'))
        if self.eventdt.year < 2000:
            raise UnsupportedDataError('Event is too old')

        if event.getElementsByTagName('Duration') != []:
            duration = event.getElementsByTagName('Duration')[0]
            def tryint(s):
                if s == '':
                    return 0
                else:
                    return int(s)
            weeks = tryint(getXMLField(duration, 'Weeks'))
            days = tryint(getXMLField(duration, 'Days'))
            hours = tryint(getXMLField(duration, 'Hours'))
            mins = tryint(getXMLField(duration, 'Minutes'))
            secs = tryint(getXMLField(duration, 'Seconds'))
            self.duration = datetime.timedelta(weeks * 7 + days, (hours * 60 + mins) * 60 + secs)
        else:
            endstr = getField('DateEnd')
            if endstr != '':
                self.duration = parse_ical_time(endstr) - self.eventdt
            else:
                # no duration or end specified, assume whole-day or no duration
                if isinstance(self.eventdt, date):
                    self.duration = timedelta(1)
                else:
                    self.duration = timedelta(0)

        # for some reason I don't understand, the phone only allows events
        # longer than a day if the time flag is set. pander to this by forcing 
        # such events to start at midnight local time
        if isinstance(self.eventdt, date) and self.duration > timedelta(1):
            local_midnight = datetime_time(0, 0, 0, 0, dateutil.tz.tzlocal())
            self.eventdt = datetime.combine(self.eventdt, local_midnight)

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
        exdates = event.getElementsByTagName('ExceptionDateTime')
        exrules = event.getElementsByTagName('ExceptionRule')
        (self.repeat_type, self.exceptions) = convert_rrule(rrules, exdates,
                                                          exrules, self.eventdt)

        if len(rrules) > 0 and self.repeat_type == MOTO_REPEAT_NONE:
            raise UnsupportedDataError('Unhandled recursion rule')


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
            for node in child.child_xml(doc):
                top.appendChild(node)

        return doc.toxml()

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
            nameparts = [getField('Name', p) for p in XML_NAME_PARTS]
            nameparts = [p != '' for p in nameparts]
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

        # process telephone numbers and email addresses
        # create a list [(contacttype, contact, is_pref)]
        contacts = []
        seen_pref = False # have we already seen a preferred contact?
        for elt in getElementsByTagNames(doc, set(['Telephone', 'EMail']), []):
            content = getXMLField(elt, 'Content')
            if elt.tagName == 'Telephone':
                # filter out any illegal characters from the phone number
                content = filter(lambda c: c in TEL_NUM_DIGITS, content)
                ical_types = elt.getAttribute('Type').split(';')
                is_pref = elt.hasAttribute('Preferred') and elt.getAttribute('Preferred') in ['1', 'true']
                moto_type = MOTO_CONTACT_DEFAULT
                for t in ical_types:
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
        # FIXME: addresses that don't map to moto contact types are dropped
        addresses = {}
        for adr in doc.getElementsByTagName('Address'):
            address = [getXMLField(adr, p) for p in XML_ADDRESS_PARTS]
            ical_types = adr.getAttribute('Location').split(';')
            for t in ical_types:
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
        if self.address:
            (street1, street2, city, state, postcode, country) = self.address
        else:
            street1 = street2 = city = state = postcode = country = ''
        return (self.pos, self.contact, self.numtype, self.parent.name,
                self.contacttype, self.voicetag, self.ringerid, 0,
                int(self.primaryflag), self.parent.categorynum, 
                self.profile_icon, self.parent.firstlast_enabled,
                self.parent.firstlast_index, self.picture_path,
                0, 0, street2, street1, city, state, postcode, country,
                self.parent.nickname, birthdaystr)

    def child_xml(self, doc):
        """Return XML nodes for this child's data."""
        ret = []

        if self.contacttype == MOTO_CONTACT_EMAIL:
            e = doc.createElement('EMail')
        elif self.contacttype == MOTO_CONTACT_MAILINGLIST:
            # the 'contact' is a space-separated list of other contact positions
            assert(False, "mailing lists aren't handled yet, sorry") # FIXME
        else:
            e = doc.createElement('Telephone')
            if MOTO_PHONE_CONTACT_TYPES.has_key(self.contacttype):
                e.setAttribute('Type', MOTO_PHONE_CONTACT_TYPES[self.contacttype])
            elif MOTO_PHONE_CONTACT_LOCATIONS.has_key(self.contacttype):
                e.setAttribute('Location', MOTO_PHONE_CONTACT_LOCATIONS[self.contacttype])
            if self.primaryflag:
                e.setAttribute('Preferred', "1")
        appendXMLChild(doc, e, 'Content', self.contact)
        ret.append(e)

        if self.address:
            e = doc.createElement('Address')
            for (part, val) in zip(XML_ADDRESS_PARTS, self.address):
                appendXMLChild(doc, e, part, val)
            if e.hasChildNodes():
                if MOTO_ADDRESS_TYPES.has_key(self.contacttype):
                    e.setAttribute('Location',
                                   MOTO_ADDRESS_TYPES[self.contacttype].upper())
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
                raise opensync.Error(desc + ' feature not present',
                                     opensync.ERROR_NOT_SUPPORTED)

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
        self.positions['event'] = PosAllocator('event', 0,
                                               self.comms.max_events - 1)
        min_contact = max(self.comms.min_contact_pos, MOTO_QUICKDIAL_ENTRIES)
        self.positions['contact'] = PosAllocator('contact', min_contact,
                                                 self.comms.max_contact_pos)

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
            entries = [PhoneEventMoto(d) for d in self.comms.read_events()]
        else:
            assert(False, 'Unknown objtype %s' % objtype)

        ret = []
        for entry in entries:
            if objtype == 'contact':
                xmldata = entry.to_xml(self.categories)
            else:
                xmldata = entry.to_xml()
            print xmldata
            data = opensync.Data(xmldata, self.objformats[objtype])
            data.objtype = objtype
            change = opensync.Change()
            change.data = data
            change.uid = self.__generate_uid(entry)
            change.hash = self.__gen_hash(entry)
            ret.append(change)
            self.positions[objtype].mark_used(entry.get_pos())
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
        self.positions[objtype].mark_free(positions)
        return True

    def update_entry(self, change):
        """Update an entry or add a new one, from the OSyncChange object.
        
        Returns True on success, False otherwise.
        """
        objtype = change.objtype
        if change.objformat != self.objformats[objtype]:
            raise opensync.Error("unhandled data format "+change.objformat.name,
                                 opensync.ERROR_NOT_SUPPORTED)
        try:
            if objtype == 'event':
                entry = PhoneEventXML(change.data.data)
            elif objtype == 'contact':
                entry = PhoneContactXML(change.data.data, self.revcategories)
        except UnsupportedDataError, e:
            warning("%s is unsupported (%s), ignored" % (change.uid, str(e)))
            # we have an entry that can't be stored on the phone
            # if its modified and we've seen it before, delete it
            # otherwise just ignore it
            if (change.changetype == opensync.CHANGE_TYPE_MODIFIED
                and self.uid_seen(change.uid)):
                change.changetype = opensync.CHANGE_TYPE_DELETED
                change.data = None
                return self.delete_entry(change.uid)
            else:
                return False
        
        if change.changetype == opensync.CHANGE_TYPE_ADDED:
            # allocate positions for the new entry
            positions = self.positions[objtype].alloc(entry.num_pos())
            entry.set_pos(positions)
        else:
            _, positions = self.__uid_to_pos(change.uid)
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

    def uid_seen(self, uid):
        """Returns true iff the given UID is one of ours."""
        try:
            self.__uid_to_pos(uid)
            return True
        except (AssertionError, ValueError, TypeError):
            return False

    def __init_categories(self):
        """Initialise a hash and reverse hash of category IDs."""
        # for each category we get: pos,name,other junk
        for data in self.comms.read_categories():
            catid, catname = data[:2]
            self.categories[catid] = catname
            self.revcategories[catname.lower()] = catid

    def __gen_hash(self, entry):
        """generate hash for the opensync hashtable, md5 of all the data"""
        m = md5.new()
        for item in entry.hash_data():
            m.update(unicode(item).encode('utf7'))
        return m.hexdigest()

    def __generate_uid(self, entry):
        """Generate a "hopefully unique" UID for an entry.

        Uses the last 8 digit's of the phone's IMEI to do so.
        """
        return ("moto-%s-%s@%s"
                % (entry.get_objtype(), entry.generate_uid(), self.serial[-8:]))

    def __uid_to_pos(self, uid):
        """Reverse the generate_uid function above.
        
        Also checks that it is one of ours.
        """
        moto, objtype, lastpart = uid.split('-', 2)
        assert(moto == "moto" and objtype in SUPPORTED_OBJTYPES,
               'Invalid UID: %s' % uid)
        lastpos = lastpart.rindex('@')
        assert(lastpart[lastpos + 1:] == self.serial[-8:],
               'Entry not created on this phone')
        
        if objtype == "event":
            positions = PhoneEvent.unpack_uid(lastpart[:lastpos])
        elif objtype == "contact":
            positions = PhoneContact.unpack_uid(lastpart[:lastpos])
        return objtype, positions


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

    def get_changes(self, info, ctx):
        """Report all OSyncChange objects for entries on the phone."""
        if self.sink.slowsync:
            self.hashtable.reset()
        for change in self.access.list_changes(self.objtype):
            self.hashtable.report(change.uid)
            change.changetype = self.hashtable.get_changetype(change.uid,
                                                              change.hash)
            if change.changetype != opensync.CHANGE_TYPE_UNMODIFIED:
                self.hashtable.update_hash(change.changetype, change.uid,
                                           change.hash)
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
            raise opensync.Error('unsupported objtype %s' % change.objtype,
                                 opensync.ERROR_NOT_SUPPORTED)
        if change.changetype != opensync.CHANGE_TYPE_DELETED:
            print "\nOpenSync wants me to write:\n", change.data.data
        if change.changetype == opensync.CHANGE_TYPE_DELETED:
            success = (self.access.uid_seen(change.uid)
                       and self.access.delete_entry(change.uid))
        elif change.changetype == opensync.CHANGE_TYPE_MODIFIED:
            old_uid = change.uid
            success = self.access.update_entry(change)
            # if the UID has changed, we need to tell our hashtable that
            # the old one was deleted, to keep it consistent
            if (success and old_uid != change.uid):
                self.hashtable.update_hash(opensync.CHANGE_TYPE_DELETED,
                                           old_uid, None)
        else:
            success = self.access.update_entry(change)
        if success:
            self.hashtable.update_hash(change.changetype, change.uid,
                                       change.hash)

    def disconnect(self, info, ctx):
        """Called to disconnect from the phone."""
        self.access.disconnect()


def parse_config(configstr):
    """Parse the config data and return the device string."""
    try:
        doc = xml.dom.minidom.parseString(configstr)
    except:
        raise opensync.Error('failed to parse config data',
                             opensync.ERROR_MISCONFIGURATION)

    ret = getXMLField(doc, 'device').strip()
    if ret == '':
        raise opensync.Error('device not specified in config file',
                             opensync.ERROR_MISCONFIGURATION)
    return ret

def initialize(info):
    """Called by python-module plugin wrapper, registers sync classes."""
    comms = PhoneComms(parse_config(info.config))
    access = PhoneAccess(comms, info)
    for objtype in SUPPORTED_OBJTYPES:
        info.add_objtype(MotoSink(objtype, info, access).sink)

def discover(info):
    """Called by python-module wrapper, discovers capabilities of device."""
    # HACK HACK, grab the comms object out of the initialised sink
    comms = info.nth_objtype(0).callback_obj.access.comms
    comms.connect()
    version = opensync.Version()
    version.plugin = "moto-sync"
    version.softwareversion = str(comms.read_version())
    version.modelversion = str(comms.read_model())
    version.identifier = str(comms.read_serial())
    comms.disconnect()
    info.version = version
    info.capabilities = opensync.capabilities_parse(CAPABILITIES)

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
