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

import sys, os, types, traceback, md5, time, calendar
import xml.dom.minidom
from datetime import date, datetime, timedelta, tzinfo
import icalendar # from http://codespeak.net/icalendar/
import opensync

# FIXME: error/exception handling in this module needs to be much better

# debug options:
DEBUG_OUTPUT = True # if enabled, this prints all interaction with the phone to stdout
WRITE_ENABLED = False # if disabled, prevents any changes from being made on the phone

# object types supported by this plugin
SUPPORTED_OBJTYPES = ['event', 'contact']

# my phone doesn't like it if you ask to read more than this many entries at a time
ENTRIES_PER_READ = 15

# time and date formats
PHONE_TIME = '%H:%M'
PHONE_DATE = '%m-%d-%Y' # yuck!
VCAL_DATETIME = '%Y%m%dT%H%M%S'
VCAL_DATE = '%Y%m%d'

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

# reverse of the above (almost): mapping from vcard number types to the phone's contact type
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
# FIXME: can also have dom / intl / postal / parcel... these don't really make sense here
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
XML_ADDRESS_PARTS = 'Street ExtendedAddress City Region PostalCode Country'.split()


# UTC tzinfo class, stolen from python-icalendar
class UTC(tzinfo):
    def utcoffset(self, dt):
        return timedelta(0)

    def tzname(self, dt):
        return "UTC"

    def dst(self, dt):
        return timedelta(0)
UTC = UTC()


# utility functions for various XML parsing stuff below
def getXMLField(doc, tagname, subtag=None):
    """Returns text in a given XML tag, or '' if not set.

    The XML structure that it looks for is: <tagname><subtag>text here</subtag></tagname>.
    It always returns the data in the first subtag of the first tag, repeated tags are ignored.
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
    """Append a child tag with the given text content to the given parent node."""
    if text and text != '':
        e = doc.createElement(tag)
        e.appendChild(doc.createTextNode(text.encode('utf8')))
        parent.appendChild(e)


class OpenSyncError(Exception):
    """Simple exception class carrying a message and an opensync error number.

    These errors are reported back to opensync by the stdexceptions decorator on SyncClass methods.
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

        # reset the phone and send it a bunch of init strings
        self.__do_cmd('AT&F')      # reset to factory defaults
        self.__do_cmd('AT+MODE=0') # ?
        self.__do_cmd('ATE0Q0V1')  # echo off, result codes off, verbose results

        # use ISO 8859-1 encoding for data values, for easier debugging (FIXME: change to UCS2?)
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
        (maxevs, numevs, titlelen, exmax, extypemax) = self.__parse_results('MDBR', data)[0]
        maxevents = int(maxevs)
        numevents = int(numevs)

        # read entries from the phone until we've seen all of them
        ret = []
        pos = 0
        while pos < maxevents and len(ret) < numevents:
            end = min(pos + ENTRIES_PER_READ - 1, maxevents - 1)
            data = self.__do_cmd('AT+MDBR=%d,%d' % (pos, end))

            # first parse all the exceptions for each event
            exceptions = {}
            for (expos, exnum, extype) in self.__parse_results('MDBRE', data):
                expos = int(expos)
                exnum = int(exnum)
                extype = int(extype)
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
        if WRITE_ENABLED:
            self.__do_cmd('AT+MDBW=%d,"%s",%d,%d,"%s","%s",%d,"%s","%s",%d' % data)
            for expos in exceptions:
                self.__do_cmd('AT+MDBWE=%d,%d,1' % (pos, expos))

    def delete_event(self, pos):
        """delete the event at a specific position"""
        self.open_calendar()
        if WRITE_ENABLED:
            self.__do_cmd('AT+MDBWE=%d,0,0' % pos)

    def read_categories(self):
        """Get list of category IDs/names for the phonebook."""
        self.close_calendar()
        # open phone memory
        self.__do_cmd('AT+CPBS="ME"')

        # read groups
        data = self.__do_cmd('AT+MPGR=?')
        (mingroup, maxgroup) = self.__parse_range(self.__parse_results('MPGR', data)[0][0])
        ret = []
        pos = mingroup
        while pos <= maxgroup:
            end = min(pos + ENTRIES_PER_READ - 1, maxgroup)
            data = self.__do_cmd('AT+MPGR=%d,%d' % (pos, end))
            ret.extend(self.__parse_results('MPGR', data))
            pos += ENTRIES_PER_READ

        return ret

    def read_contacts(self):
        """read a list of all contacts in the phonebook"""
        self.close_calendar()
        # open phone memory
        self.__do_cmd('AT+CPBS="ME"')

        # read parameters
        data = self.__do_cmd('AT+CPBR=?')
        (rangestr, numberlenstr, namelenstr) = self.__parse_results('CPBR', data)[0]
        (minpos, maxpos) = self.__parse_range(rangestr)
        numberlen = int(numberlenstr)
        namelen = int(namelenstr)

        # check usage
        data = self.__do_cmd('AT+CPBS?')
        result = self.__parse_results('CPBS', data)[0]
        assert(result[0] == 'ME')

        if len(result) >= 3:
            # newer phones gives us three values
            (memtype, inusestr, maxusedstr) = result[:3]
            assert(memtype == 'ME')
            maxused = int(maxusedstr)
            assert(maxpos - minpos + 1 <= maxused)
            inuse = int(inusestr)
        else:
            # older phones we don't seem to know how many entries are in use, so have
            # to read all of them :(
            inuse = maxpos - minpos

        # read entries from the phone until we've seen all of them
        ret = []
        pos = minpos
        while pos <= maxpos and len(ret) < inuse:
            end = min(pos + ENTRIES_PER_READ - 1, maxpos)
            data = self.__do_cmd('AT+MPBR=%d,%d' % (pos, end))
            ret.extend(self.__parse_results('MPBR', data))
            pos += ENTRIES_PER_READ
        return ret

    def write_contact(self, data):
        """write a single contact to the position specified in the data list"""
        def make_placeholder(val):
            t = type(val)
            if t == types.StringType:
                return '"%s"'
            elif t == types.IntType:
                return '%d'
            assert(False, 'unexpected type %s' % str(t))

        self.close_calendar()
        if WRITE_ENABLED:
            self.__do_cmd('AT+MPBW=' + ','.join(map(make_placeholder, data)) % data)

    def delete_contact(self, pos):
        """delete the contact at a given position"""
        self.close_calendar()
        if WRITE_ENABLED:
            self.__do_cmd('AT+MPBW=%d' % pos)

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
            print ('<-- %s' % ret)
        if c == '': # EOF, shouldn't happen
            raise OpenSyncError('Unexpected EOF talking to phone', opensync.ERROR_IO_ERROR)
        return ret

    def __do_cmd(self, cmd):
        """Send a command to the phone and wait for its response.

        If it succeeds, return lines as a list; otherwise raise an exception.
        """
        if DEBUG_OUTPUT:
            print ('--> %s' % cmd)
        os.write(self.__fd, cmd + "\r")
        ret = []
        line = self.__readline()
        while line != 'OK' and line != 'ERROR':
            ret.append(line)
            line = self.__readline()
        if line == 'OK':
            return ret
        else:
            raise OpenSyncError("Error in phone command '%s'" % cmd, opensync.ERROR_IO_ERROR)

    def __parse_results(self, restype, lines):
        """extract results from a list of reply lines"""
        ret = []
        prefix = '+' + restype + ': '
        for line in lines:
            if line.startswith(prefix):
                # remove prefix
                line = line[len(prefix):]

                # split line into comma-separated values, removing quotes
                parts = []
                nextpart = ''
                inquote = False
                for c in line:
                    if c == '"':
                        inquote = not inquote
                    elif c == ',' and not inquote:
                        parts.append(nextpart)
                        nextpart = ''
                    else:
                        if inquote:
                            nextpart += c.decode('iso_8859_1')
                        else:
                            nextpart += c
                parts.append(nextpart)
                ret.append(parts)
        return ret

    def __parse_range(self, rangestr):
        """parse a range string like '(1-1000)'"""
        assert(rangestr[0] == '(' and rangestr[-1] == ')')
        return map(int, rangestr[1:-1].split('-',1))


class PhoneEntry:
    """(abstract) base class representing an event/contact entry roughly as stored in the phone."""
    def __init__(self, data, format):
        raise "this class cannot be instantiated"

    def get_objtype(self):
        """return the opensync object type string"""
        raise "implement me in a subclass"

    def generate_uid(self):
        """Return a UID string (or part of it) that stores the position(s) used by this entry."""
        raise "implement me in a subclass"

    @staticmethod
    def unpack_uid(uid):
        """Unpack a UID string returned by generate_uid to a list of positions."""
        raise "implement me in a subclass"

    def num_pos(self):
        """Return the number of positions occupied by this entry."""
        raise "implement me in a subclass"

    def set_pos(self, positions):
        """Set the positions occupied by this entry."""
        raise "implement me in a subclass"

    def to_moto(self):
        """return the list of data items to be written to the phone"""
        raise "implement me in a subclass"

    def to_xml(self, arg):
        """return the opensync XML representation of this entry"""
        raise "implement me in a subclass"

    # utility functions for subclasses:
    def parse_moto_time(self, datestr, timestr=None):
        """convert phone's date and time string into a datetime object"""
        if timestr:
            t = time.strptime(datestr+' '+timestr, PHONE_DATE+' '+PHONE_TIME)
            # assume that phone time is in our local timezone, convert to UTC
            return datetime.fromtimestamp(time.mktime(t), UTC)
        else:
            t = time.strptime(datestr, PHONE_DATE)
            return date.fromtimestamp(time.mktime(t))

    def format_time(self, dt, format):
        """convert datetime object into local time and format as a string"""
        if isinstance(dt, datetime) and dt.utcoffset() == timedelta(0):
            # FIXME: assumes local timezone
            t = time.localtime(calendar.timegm(dt.utctimetuple()))
            return time.strftime(format, t)
        else:
            # already in localtime
            return dt.strftime(format)


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
            raise OpenSyncError("unhandled data format %s" % format, opensync.ERROR_NOT_SUPPORTED)

    def get_objtype(self):
        return "event"

    def generate_uid(self):
        return str(self.pos)

    @staticmethod
    def unpack_uid(uid):
        return [int(uid)]

    def num_pos(self):
        return 1

    def set_pos(self, positions):
        assert(len(positions) == 1)
        self.pos = positions[0]

    def __from_moto(self, data):
        """grab stuff out of the list of values from the phone"""
        assert(type(data) == list and len(data) == 11)
        self.pos = int(data[0])
        self.name = data[1]
        timeflag = int(data[2])
        alarmflag = int(data[3])
        time = data[4]
        date = data[5]
        self.duration = timedelta(0, 0, 0, 0, int(data[6]))
        alarmtime = data[7]
        alarmdate = data[8]
        self.repeat_type = int(data[9])
        self.exceptions = data[10]

        if timeflag:
            self.eventdt = self.parse_moto_time(date, time)
        else:
            self.eventdt = self.parse_moto_time(date)

        if alarmflag:
            self.alarmdt = self.parse_moto_time(alarmdate, alarmtime)
        else:
            self.alarmdt = None

    # FIXME: handle exceptions if parameters don't exist or we can't parse the data
    def __from_xml(self, data):
        """parse xml event"""
        doc = xml.dom.minidom.parseString(data)
        event = doc.getElementsByTagName('Event')[0]

        # utility function for the XML processing below
        def getField(tagname, subtag='Content'):
            return getXMLField(event, tagname, subtag)

        self.pos = None
        self.name = getField('Summary')

        self.eventdt = icalendar.vDDDTypes.from_ical(getField('DateStarted'))
        durationstr = getField('Duration')
        if durationstr != '':
            self.duration = icalendar.vDDDTypes.from_ical(durationstr)
        else:
            self.duration = icalendar.vDDDTypes.from_ical(getField('DateEnd')) - self.eventdt

        triggerstr = getField('Alarm', 'AlarmTrigger')
        if triggerstr == '':
            self.alarmdt = None
        else:
            trigger = icalendar.vDDDTypes.from_ical(triggerstr)
            if isinstance(trigger, timedelta):
                self.alarmdt = self.eventdt + trigger
            else:
                self.alarmdt = trigger

        rrule = getField('RecurrenceRule', 'Rule')
        if rrule != '':
            # FIXME: process RRULE!
            self.repeat_type = MOTO_REPEAT_NONE # failed conversion
        else:
            self.repeat_type = MOTO_REPEAT_NONE

        self.exceptions = [] # FIXME!

    def to_moto(self):
        """generate motorola event-data list"""
        if isinstance(self.eventdt, datetime):
            timeflag = 1
            datestr = self.format_time(self.eventdt, PHONE_DATE)
            timestr = self.format_time(self.eventdt, PHONE_TIME)
        else:
            timeflag = 0
            datestr = self.eventdt.strftime(PHONE_DATE)
            timestr = '00:00'
        if self.alarmdt:
            alarmflag = 1
            alarmdatestr = self.format_time(self.alarmdt, PHONE_DATE)
            alarmtimestr = self.format_time(self.alarmdt, PHONE_TIME)
        else:
            alarmflag = 0
            alarmdatestr = '00-00-2000'
            alarmtimestr = '00:00'

        duration = int(self.duration.days) * 24 * 60
        duration += int(self.duration.seconds) / 60
        return (self.pos, self.name, timeflag, alarmflag, timestr, datestr,
                duration, alarmtimestr, alarmdatestr, self.repeat_type,
                self.exceptions)

    def to_xml(self, ignored):
        doc = xml.dom.minidom.getDOMImplementation().createDocument(None, 'vcal', None)
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
        appendXMLChild(doc, e, 'Content', (self.eventdt + self.duration).strftime(VCAL_DATETIME))
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
            day = ['MO','TU','WE','TH','FR','SA','SU'][self.eventdt.weekday()]
            # compute the week number that the event falls in
            if self.eventdt.day % 7 == 0:
                wk = self.eventdt.day / 7
            else:
                wk = self.eventdt.day / 7 + 1
            appendXMLChild(doc, e, 'Rule', 'BYDAY=%d%s' % (wk, day))
        elif self.repeat_type == MOTO_REPEAT_YEARLY:
            appendXMLChild(doc, e, 'Rule', 'FREQ=YEARLY')

        if e.hasChildNodes():
            top.appendChild(e)

        # FIXME: exceptions!

        return doc.toxml()

class PhoneContactBase(PhoneEntry):
    """Class representing the contacts roughly as stored in the phone.

    PhoneContactBase represents the common fields of a logical identity that
    may be split across several entries in the phone book for different contact types.

    class members:
       children:        list of PhoneContact objects, which contain attributes unique to each entry
       name:            name of this entry
       categorynum:     category to which this entry belongs
       firstlast_enabled/
       firstlast_index: subfield information about name (where FirstName and LastName are)
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
            raise OpenSyncError("unhandled data format %s" % format, opensync.ERROR_NOT_SUPPORTED)

    def get_objtype(self):
        return "contact"

    def generate_uid(self):
        return ','.join(map(lambda c: str(c.pos), self.children))

    @staticmethod
    def unpack_uid(uid):
        return map(int, uid.split(','))

    def num_pos(self):
        return len(self.children)

    def set_pos(self, positions):
        assert(len(positions) == len(self.children))
        for (p, c) in zip(positions, self.children):
            c.pos = p

    def __from_moto(self, data):
        """grab stuff out of the list of values from the phone"""
        assert(type(data) == list and len(data) >= 24)
        self.name = data[3]
        self.categorynum = int(data[9])
        self.firstlast_enabled = int(data[11]) #0 firstname lastname, 1 lastname firstname, 255 unknown
        self.firstlast_index = int(data[12]) # 0-based index of second field (ignored if enabled=255)
        self.nickname = data[22]
        if data[23] == '':
            self.birthday = None
        else:
            self.birthday = self.parse_moto_time(data[23])

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
            if other.firstlast_enabled != MOTO_INVALID and self.firstlast_enabled == MOTO_INVALID:
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
            namelist = filter(lambda x: x != '', map(lambda p: getField('Name', p), XML_NAME_PARTS))
            self.name = ' '.join(namelist)
            self.firstlast_enabled = 0
            self.firstlast_index = self.name.index(getField('Name', 'LastName'))

        self.categorynum = revcategories.get(getField('Categories', 'Category').lower(), MOTO_CATEGORY_DEFAULT)
        self.nickname = getField('Nickname')
        bdaystr = getField('Birthday')
        if bdaystr != '':
            self.birthday = icalendar.vDDDTypes.from_ical(bdaystr)
        else:
            self.birthday = None

        # NOW WE PROCESS ALL THE CONTACT INFO AND CREATE CHILDREN

        # we always have one (uninitialised) child to start with, use it first
        assert(len(self.children) == 1)
        self.firstchild_used = False

        # utility function for creating children
        def makeChild(contact, contacttype, address=None):
            if self.firstchild_used:
                child = PhoneContact((self, contact, contacttype, 0, address), "XXX-parentcontact-hack")
                self.children.append(child)
            else:
                self.children[0].from_parent((self, contact, contacttype, 1, address))
                self.firstchild_used = True

        # process telephone numbers, create a list [(contacttype, tel)] with preferred number first
        telephones = []
        for tel in doc.getElementsByTagName('Telephone'):
            telephone = getXMLField(tel, 'Content')
            types = map(lambda e: getXMLText(e).lower(), tel.getElementsByTagName('Type'))
            moto_type = MOTO_CONTACT_DEFAULT
            for t in types:
                if VCARD_CONTACT_TYPES.has_key(t):
                    moto_type = VCARD_CONTACT_TYPES[t]
                    break
            # make a preferred entry come first
            if 'pref' in types:
                telephones.insert(0, (moto_type, telephone))
            else:
                telephones.append((moto_type, telephone))

        # process addresses, create a hash from contacttype to address
        # FIXME: addresses that don't map cleanly to motorola contact types are silently dropped
        addresses = {}
        for adr in doc.getElementsByTagName('Address'):
            address = map(lambda p: getXMLField(adr, p), XML_ADDRESS_PARTS)
            types = map(lambda e: getXMLText(e).lower(), adr.getElementsByTagName('Type'))
            for t in types:
                if VCARD_ADDRESS_TYPES.has_key(t):
                    moto_type = VCARD_ADDRESS_TYPES[t]
                    addresses[moto_type] = address
                    break

        # create a child for each telephone/address pair
        for (moto_type, telephone) in telephones:
            makeChild(telephone, moto_type, addresses.get(moto_type, None))

        # create children for all the email addresses
        for e in doc.getElementsByTagName('EMail'):
            makeChild(getXMLField(e, 'Content'), MOTO_CONTACT_EMAIL)

        assert(self.firstchild_used) # we should have made at least one child

    def to_xml(self, categories):
        doc = xml.dom.minidom.getDOMImplementation().createDocument(None, 'contact', None)
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
            e = doc.createElement('Birthday')
            appendXMLChild(doc, e, 'Content', self.format_time(self.birthday, VCAL_DATE))
            top.appendChild(e)

        e = doc.createElement('Categories')
        appendXMLChild(doc, e, 'Category', categories[self.categorynum])
        top.appendChild(e)

        for child in self.children:
            for node in child.child_xml(doc):
                top.appendChild(node)

        return doc.toxml()


class PhoneContact:
    """Class representing the contacts roughly as stored in the phone.

    PhoneContactBase represents the common fields of a logical identity that
    may be split across several entries in the phone book for different contact types.

    class members:
       parent:          parent PhoneContactBase object
       pos:             position of this entry in the phone book
       contact:         contact data (number or email address)
       contacttype:     integer representing contact type (home/work/etc., email)
       numtype:         one of MOTO_NUMTYPE_*
       voicetag:        flag if voice-dial tag is set for this entry
       ringerid:        ringer ID for this entry
       primaryflag:    flag set on the "primary" contact for a given identity
       profile_icon:    profile icon number
       picture_path:    path to picture for this entry (on the phone's filesystem)
       address:         None, or list of address parts
    """
    def __init__(self, data, format, revcategories=None):
        if format == "XXX-parentcontact-hack":
            self.from_parent(data)
        else:
            if format == "moto-contact":
                self.__from_moto(data)
            self.parent = PhoneContactBase(self, data, format, revcategories)

    def __getattr__(self, name):
        """If we don't have an attribute, return the parent's version that
        is common to all of its children.
        """
        return getattr(self.parent, name)

    def to_moto(self):
        """Generate motorola contact-data list."""
        if self.birthday:
            birthdaystr = self.format_time(self.birthday, PHONE_DATE)
        else:
            birthdaystr = ''
        if self.address:
            (street1, street2, city, state, postcode, country) = self.address
        else:
            street1 = street2 = city = state = postcode = country = ''
        return (self.pos, self.contact, self.numtype, self.name, self.contacttype,
                self.voicetag, self.ringerid, 0, self.primaryflag,
                self.categorynum, self.profile_icon, self.firstlast_enabled,
                self.firstlast_index, self.picture_path, 0, 0, street2, street1,
                city, state, postcode, country, self.nickname, birthdaystr)

    def __from_moto(self, data):
        """grab stuff out of the list of values from the phone"""
        assert(type(data) == list and len(data) >= 24)
        self.pos = int(data[0])
        self.contact = data[1]
        self.numtype = int(data[2])
        self.contacttype = int(data[4])
        self.voicetag = int(data[5])
        self.ringerid = int(data[6])
        assert(int(data[7]) == 0) # backlight flag?
        self.primaryflag = int(data[8])
        self.profile_icon = int(data[10])
        self.picture_path = data[13]
        assert(int(data[14]) == 0) # unknown?
        assert(int(data[15]) == 0) # unknown?
        self.address = (data[17], data[16], data[18], data[19], data[20], data[21])
        # assert(data[24] == '') # unknown? old phones don't have it

    def child_xml(self, doc):
        ret = []

        if self.contacttype == MOTO_CONTACT_EMAIL:
            e = doc.createElement('EMail')
        elif self.contacttype == MOTO_CONTACT_MAILINGLIST:
            # the 'contact' is a space-separated list of other contact positions
            assert(0) # FIXME
        else:
            e = doc.createElement('Telephone')
            appendXMLChild(doc, e, 'Type', MOTO_CONTACT_TYPES[self.contacttype])
            if self.primaryflag:
                appendXMLChild(doc, e, 'Type', 'pref')
        appendXMLChild(doc, e, 'Content', self.contact)
        ret.append(e)

        if self.address:
            e = doc.createElement('Address')
            for (part, val) in zip(XML_ADDRESS_PARTS, self.address):
                appendXMLChild(doc, e, part, val)
            if e.hasChildNodes():
                if MOTO_ADDRESS_TYPES.has_key(self.contacttype):
                    appendXMLChild(doc, e, 'Type', MOTO_ADDRESS_TYPES[self.contacttype])
                ret.append(e)

        return ret

    def from_parent(self, data):
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
    """Grab-bag class of utility functions, interface between PhoneComms, PhoneEntry objects and SyncClass below."""
    def __init__(self, comms):
        self.comms = comms
        self.sn = comms.read_serial()
        self.positions_used = {}
        for objtype in SUPPORTED_OBJTYPES:
            self.positions_used[objtype] = []

        # check that the phone supports the features we need
        features = comms.read_features()
        for (bit, desc) in REQUIRED_FEATURES:
            if not features[bit]:
                raise OpenSyncError(desc + ' feature not present', opensync.ERROR_NOT_SUPPORTED)

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

    def list_changes(self):
        """Return a list of OSyncChange objects for all events."""
        ret = []
        self.__init_categories()

        # sort contacts by name, and attempt to merge adjacent ones
        contacts = map(lambda d: PhoneContact(d, 'moto-contact', self.revcategories), self.comms.read_contacts())
        contacts.sort(key=lambda c: c.name)
        i = 0
        while i < (len(contacts) - 1):
            if contacts[i].parent.merge(contacts[i + 1].parent):
                del contacts[i + 1]
            else:
                i += 1

        events = map(lambda d: PhoneEvent(d, 'moto-event'), self.comms.read_events())

        for entry in contacts + events:
            change = opensync.OSyncChange()
            change.objtype = objtype = entry.get_objtype()
            change.uid = self.__generate_uid(entry)
            change.format = "xml-%s-doc" % objtype
            change.data = entry.to_xml(self.categories)
            change.hash = self.__gen_hash(change.data)
            ret.append(change)
            self.positions_used[objtype].append(entry.pos)
        return ret

    def delete_entry(self, uid):
        """delete an event with the given UID"""
        objtype, positions = self.__uid_to_pos(uid)
        for pos in positions:
            if objtype == 'event':
                self.comms.delete_event(pos)
            elif objtype == 'contact':
                self.comms.delete_contact(pos)
            self.positions_used[objtype].remove(pos)

    def update_entry(self, change):
        """update an entry, or add a new one, from the given OSyncChange object"""
        if change.objtype == 'event':
            entry = PhoneEvent(change.data, change.format)
        elif change.objtype == 'contact':
            entry = PhoneContact(change.data, change.format, self.revcategories)
        if change.changetype == opensync.CHANGE_ADDED:
            entry.set_pos(self.__get_free_positions(entry.get_objtype(), entry.num_pos()))
            change.uid = self.__generate_uid(entry)
        else:
            _, positions = self.__uid_to_pos(change.uid)
            entry.set_pos(positions)
        change.hash = self.__gen_hash(entry.to_xml(self.categories))
        if change.objtype == 'event':
            self.comms.write_event(entry.to_moto())
        elif change.objtype == 'contact':
            self.comms.write_contact(entry.to_moto())

    def __init_categories(self):
        """Initialise a hash and reverse hash of category IDs."""
        self.categories = {}
        self.revcategories = {}

        # for each category we get: pos,name,ringer id,0,0,""
        for data in self.comms.read_categories():
            catid, catname = data[:2]
            catid = int(catid)
            self.categories[catid] = catname
            self.revcategories[catname.lower()] = catid

    def __gen_hash(self, data):
        """generate hash for the opensync hashtable, md5 of all the data"""
        m = md5.new()
        m.update(data)
        return m.hexdigest()

    def __generate_uid(self, entry):
        """Generate a "hopefully unique" UID for an entry.

        Uses the last 8 digit's of the phone's IMEI to do so.
        """
        return "moto-%s-%s@%s" % (entry.get_objtype(), entry.generate_uid(), self.sn[-8:])

    def __uid_to_pos(self, uid):
        """Reverse the generate_uid function above. Check that it is one of ours."""
        moto, objtype, lastpart = uid.split('-', 2)
        assert(moto == "moto" and objtype in SUPPORTED_OBJTYPES, 'Invalid UID: %s' % uid)
        lastpos = lastpart.rindex('@')
        assert(lastpart[lastpos + 1:] == self.sn[-8:], 'Entry not created on this phone')
        if objtype == "event":
            positions = PhoneEvent.unpack_uid(lastpart[:lastpos])
        elif objtype == "contact":
            positions = PhoneContactBase.unpack_uid(lastpart[:lastpos])
        return objtype, positions

    # FIXME: check for maximum number of events in the phone
    def __get_free_positions(self, objtype, num_pos):
        """Allocate num_pos free positions for a new event."""
        i = 0
        ret = []
        used = self.positions_used[objtype]
        while len(ret) < num_pos:
            while i < len(used) and used[i] == i:
                i += 1
            used.insert(i, i)
            ret.append(i)
        return ret


def stdexceptions(func):
    """Decorator used to wrap every method in SyncClass with the same exception handlers."""
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
            raise OpenSyncError('hashtable load failed', opensync.ERROR_INITIALIZATION)

        self.comms = PhoneComms(self.config.device)
        self.access = PhoneAccess(self.comms)

    @stdexceptions
    def get_changeinfo(self, ctx):
        for objtype in SUPPORTED_OBJTYPES:
            if self.member.get_slow_sync(objtype):
                self.hashtable.set_slow_sync(objtype)

        for change in self.access.list_changes():
            self.hashtable.detect_change(change)
            if change.changetype != opensync.CHANGE_UNMODIFIED:
                change.report(ctx)
                self.hashtable.update_hash(change)

        for objtype in SUPPORTED_OBJTYPES:
            self.hashtable.report_deleted(ctx, objtype)

    @stdexceptions
    def commit_change(self, ctx, change):
        if change.objtype not in SUPPORTED_OBJTYPES:
            raise OpenSyncError('unsupported objtype %s' % change.objtype,
                                opensync.ERROR_NOT_SUPPORTED)
        if change.changetype == opensync.CHANGE_DELETED:
            self.access.delete_entry(change.uid)
        else:
            self.access.update_entry(change)
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
            raise OpenSyncError('failed to parse config data', opensync.ERROR_MISCONFIGURATION)

        class Config:
            pass
        ret = Config()
        ret.device = getXMLField(doc, 'device')
        if ret.device == '':
            raise OpenSyncError('device not specified in config file', opensync.ERROR_MISCONFIGURATION)

        return ret


def initialize(member):
    return SyncClass(member)


def get_info(info):
    info.name = "moto-sync"
    for objtype in SUPPORTED_OBJTYPES:
        info.accept_objtype(objtype)
        info.accept_objformat(objtype, "xml-%s-doc" % objtype)


# debug code (not used when plugin is loaded by opensync)
if __name__ == "__main__" and hasattr(sys, "argv"):
    pc = PhoneComms('/dev/rfcomm0')
    pa = PhoneAccess(pc)
    if len(sys.argv) > 1 and sys.argv[1] == '--delete':
        # delete events
        for pos in range(0,40):
            pc.delete_event(pos)
    else:
        for change in pa.list_changes():
            print change.uid
            print change.data
