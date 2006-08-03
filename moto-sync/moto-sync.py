# opensync plugin for syncing to Motorola mobile phone
# only tested (and barely at that) with a Motorola L7
# Andrew Baumann <andrewb@cse.unsw.edu.au>, 2006/03

import sys, os, md5, time, calendar
from datetime import date, datetime, timedelta
import icalendar # from http://codespeak.net/icalendar/
import opensync
from opensync import * # FIXME: import * needed for opensync-python bindings

# FIXME: error/exception handling in this module needs to be much better
# most possible exceptions are currently unhandled

# time and date formats
PHONE_TIME = '%H:%M'
PHONE_DATE = '%m-%d-%Y'
VCAL_DATETIME = '%Y%m%dT%H%M%S'
VCAL_DATE = '%Y%m%d'

# device to use to access phone (FIXME: should use OpenSync config data)
PHONE_DEVICE = "/dev/rfcomm0"

# functions for directly accessing the phone
class PhoneComms:
	def __init__(self, device):
		self.calendar_open = False
		self.fd = os.open(device, os.O_RDWR)

		# reset the phone and send it a bunch of init strings
		self.do_cmd('AT&F')     # factory defaults
		self.do_cmd('ATE0Q0V1') # echo off, result codes off, verbose results

		# use ISO 8859-1 encoding for data values (FIXME: change to UCS2)
		self.do_cmd('AT+CSCS="8859-1"')

	def __del__(self):
		os.close(self.fd)

	# read the phone's serial number (IMEI)
	def read_serial(self):
		data = self.do_cmd('AT+CGSN')
		return self.parse_results('CGSN', data)[0][0]

	# read the phone's current date & time
	def read_time(self):
		data = self.do_cmd('AT+CCLK?')
		return self.parse_results('CCLK', data)[0][0]

	# open the calendar
	# this "locks" out the phone's own UI from accessing the data
	def open_calendar(self):
		assert(not self.calendar_open)
		self.do_cmd('AT+MDBL=1')
		self.calendar_open = True

	def close_calendar(self):
		assert(self.calendar_open)
		self.do_cmd('AT+MDBL=0')

	# read the list of all events on the phone
	def read_events(self):
		assert(self.calendar_open)
		ret = []

		# read calendar/event parameters
		data = self.do_cmd('AT+MDBR=?') # read event parameters
		(maxevs, numevs, titlelen, exmax, extypemax) = self.parse_results('MDBR', data)[0]
		maxevents = int(maxevs)
		numevents = int(numevs)

		# read events from the phone until we've seen all of them
		# read only 15 at a time, because this is the limit for the phone
		pos = 0
		while pos < maxevents and len(ret) < numevents:
			end = min(pos + 14, maxevents - 1)
			data = self.do_cmd('AT+MDBR=%d,%d' % (pos, end))

			# first parse all the exceptions for each event
			exceptions = {}
			for (expos, exnum, extype) in self.parse_results('MDBRE', data):
				expos = int(expos)
				exnum = int(exnum)
				extype = int(extype)
				assert(extype == 1) # haven't seen anything else
				if not exceptions.has_key(expos):
					exceptions[expos] = []
				exceptions[expos].append(exnum)
			# ...then add them into the event data
			for evdata in self.parse_results('MDBR', data):
				evdata.append(exceptions.get(pos, []))
				ret.append(evdata)
			pos += 15
		return ret

	# write a single event to the phone
	# uses specified pos given in event (overwriting anything on the phone)
	def write_event(self, evdata):
		assert(self.calendar_open)
		pos = evdata[0]
		self.delete_event(pos)
		exceptions = evdata[-1]
		data = evdata[:-1]
		self.do_cmd('AT+MDBW=%d,"%s",%d,%d,"%s","%s",%d,"%s","%s",%d' % data)
		for expos in exceptions:
			self.do_cmd('AT+MDBWE=%d,%d,1' % (pos, expos))

	# delete the event at a specific position
	def delete_event(self, pos):
		assert(self.calendar_open)
		self.do_cmd('AT+MDBWE=%d,0,0' % pos)

	# private function, read the next line of text from the phone
	def readline(self):
		ret = ''
		c = os.read(self.fd, 1)
		while c == '\r' or c == '\n':
			c = os.read(self.fd, 1)
		while c != '\r' and c != '\n' and c != '':
			ret += c
			c = os.read(self.fd, 1)
		print ('<-- %s' % ret)
		assert(c != '') # EOF, shouldn't happen
		return ret

	# private function, send a command to the phone and wait for its response
	# if it succeeds, return lines as a list; otherwise raise an exception
	def do_cmd(self, cmd):
		print ('--> %s' % cmd)
		os.write(self.fd, cmd + "\r\n")
		ret = []
		line = self.readline()
		while line != 'OK' and line != 'ERROR':
			ret.append(line)
			line = self.readline()
		if line == 'OK':
			return ret
		else:
			raise RuntimeError("Error in phone command")

	# private function, extract results from a list of reply lines
	def parse_results(self, restype, lines):
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
						nextpart += c
				if nextpart != '':
					parts.append(nextpart)
				ret.append(parts)
		return ret

# class representing the events roughly as stored in the phone
# class members:
#    pos:         integer position/slot in phone memory
#    name:        name/summary of event
#    eventdt:     datetime or date object (if no time is set) for event
#    duration:    timedelta object for duration of event
#    alarmdt:     datetime object or None, for alarm time
#    repeat_type: integer 0-5 for repeating events, see comment on to_ical()
#    exceptions:  list of occurrences (0-based) which do not actually happen
class PhoneEvent:
	def __init__(self, data, format):
		if format == "moto-event":
			self.from_moto(data)
		elif format == "vevent20":
			self.from_ical(data)
		else:
			raise RuntimeError("unhandled data format %s" % format)

	# grab stuff out of the list of values from the phone
	def from_moto(self, data):
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
			self.eventdt = self.parse_time(date, time)
		else:
			self.eventdt = self.parse_time(date)

		if alarmflag:
			self.alarmdt = self.parse_time(alarmdate, alarmtime)
		else:
			self.alarmdt = None

	# parse ical event
	# FIXME: handle exceptions if parameters don't exist
	def from_ical(self, data):
		cal = icalendar.Calendar.from_string(data)
		event = cal.walk('vevent')[0]
		self.pos = None
		self.name = str(event['summary'])

		self.eventdt = event.decoded('dtstart')
		if event.has_key('duration'):
			self.duration = event.decoded('duration')
		else:
			self.duration = event.decoded('dtend') - self.eventdt

		alarms = event.walk('valarm')
		if len(alarms) == 0:
			self.alarmdt = None
		else:
			trigger = alarms[0].decoded('trigger')
			if isinstance(trigger, timedelta):
				self.alarmdt = self.eventdt + trigger
			else:
				self.alarmdt = trigger

		if event.has_key('rrule'):
			# FIXME: almost totally bollocks rrule processing
			rrule = event.decoded('rrule')
			freq = rrule['FREQ'][0]
			if freq == 'DAILY':
				self.repeat_type = 1
			elif freq == 'WEEKLY':
				self.repeat_type = 2
			elif freq == 'MONTHLY':
				if rrule.has_key('BYDAY'):
					self.repeat_type = 4
				else:
					self.repeat_type = 3
			elif freq == 'YEARLY':
				self.repeat_type = 5
			else:
				self.repeat_type = 0 # failed conversion
		else:
			self.repeat_type = 0

		self.exceptions = [] # FIXME!

	# generate hash for the opensync hashtable, md5 of all the data
	def gen_hash(self):
		m = md5.new()
		m.update(self.name)
		m.update(self.eventdt.ctime())
		if self.alarmdt:
			m.update(self.alarmdt.ctime())
		else:
			m.update(datetime(2000,1,1).ctime())
		m.update("%d %d" % (self.duration.days, self.duration.seconds / 60))
		m.update(str(self.repeat_type))
		return m.hexdigest()

	# generate motorola event-data list
	def to_moto(self):
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

	# generate sucky vcalendar format (see RFC 2445)
	def to_ical(self):
		cal = icalendar.Calendar()
		cal.add('prodid', '-//abhacks//NONSGML moto-sync//')
		cal.add('version', '2.0')
		
		if self.repeat_type == 1:
			recur = icalendar.vRecur(freq='daily')
		elif self.repeat_type == 2:
			recur = icalendar.vRecur(freq='weekly')
		elif self.repeat_type == 3: # monthly on date
			recur = icalendar.vRecur(freq='monthly',
			                         bymonthday=self.eventdt.day)
		elif self.repeat_type == 4: # monthly on day
			day = ['MO','TU','WE','TH','FR','SA','SU'][self.eventdt.weekday()]
			# compute the week number that the event falls in
			if self.eventdt.day % 7 == 0:
				wk = self.eventdt.day / 7
			else:
				wk = self.eventdt.day / 7 + 1
			recur = icalendar.vRecur(freq='monthly', byday='+%d%s' % (wk,day))
		elif self.repeat_type == 5:
			recur = icalendar.vRecur(freq='yearly')
		else:
			recur = None

		# FIXME: exceptions!

		event = icalendar.Event()
		event.add('summary', self.name)
		if isinstance(self.eventdt, datetime):
			event.add('dtstart', self.eventdt)
		else:
			event.add('dtstart;value=date', self.eventdt.strftime(VCAL_DATE))
		event.add('dtend', self.eventdt + self.duration)
		if recur:
			event.add('rrule', recur)
		if self.alarmdt:
			alarm = icalendar.Alarm()
			alarm.add('action', 'DISPLAY')
			alarm.add('description', self.name)
			# FIXME: this shouldn't be necessary, bug in the icalendar module
			alarmtime = self.alarmdt.strftime(VCAL_DATETIME)
			alarm.add('trigger;value=date-time', alarmtime)
			event.add_component(alarm)

		cal.add_component(event)
		return cal.as_string()

	# private, convert phone's date and time string into a datetime object
	def parse_time(self, datestr, timestr=None):
		if timestr:
			t = time.strptime(datestr+' '+timestr, PHONE_DATE+' '+PHONE_TIME)
			# assume that phone time is in our local timezone, convert to UTC
			return datetime.fromtimestamp(time.mktime(t), icalendar.UTC)
		else:
			t = time.strptime(datestr, PHONE_DATE)
			return date.fromtimestamp(time.mktime(t))

	# private, convert datetime object into local time and format as a string
	def format_time(self, dt, format):
		if dt.utcoffset() == timedelta(0):
			# FIXME: assumes local timezone
			t = time.localtime(calendar.timegm(dt.utctimetuple()))
			return time.strftime(format, t)
		else:
			# already in localtime
			return dt.strftime(format)

# grab-bag of utility functions, interface between PhoneEvents and Opensync
class PhoneAccess:
	def __init__(self, comms):
		self.comms = comms
		self.sn = comms.read_serial()

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

	# return a list of OSyncChange objects for all events
	def list_changes(self):
		ret = []
		self.positions_used = []
		for evdata in self.comms.read_events():
			event = PhoneEvent(evdata, "moto-event")
			change = opensync.OSyncChange()
			change.objtype = "event"
			change.uid = self.generate_uid(event)
			change.hash = event.gen_hash()
			change.format = "vevent20"
			change.data = event.to_ical()
			ret.append(change)
			self.positions_used.append(event.pos)
		return ret

	# delete an event with the given UID
	def delete_event(self, uid):
		pos = self.uid_to_pos(uid)
		self.comms.delete_event(pos)
		self.positions_used.remove(pos)

	# update an event, or add a new one, from the given OSyncChange object
	def update_event(self, change):
		event = PhoneEvent(change.data, change.format)
		if change.changetype == opensync.CHANGE_ADDED:
			event.pos = self.get_free_position()
			change.uid = self.generate_uid(event)
		else:
			event.pos = self.uid_to_pos(change.uid)
		change.hash = event.gen_hash()
		self.comms.write_event(event.to_moto())
	
	# private function, generate a "hopefully unique" UID for an event
	# uses the last 8 digit's of the phone's IMEI to do so
	def generate_uid(self, event):
		return "%d@%s.moto" % (event.pos, self.sn[-8:])

	# private function, reverse the generate_uid function above
	def uid_to_pos(self, uid):
		split = uid.split('@',1)
		# check that given uid is one of our own
		assert(len(split) == 2)
		assert(split[1] == "%s.moto" % self.sn[-8:])
		return int(split[0])

	# private function, allocate the next free position for a new event
	# FIXME: check for maximum number of events in the phone
	def get_free_position(self):
		i = 0
		while i < len(self.positions_used) and self.positions_used[i] == i:
			i += 1
		self.positions_used.insert(i, i)
		return i

# synchronisation class used by OpenSync
class SyncClass:
	def __init__(self, member):
		self.member = member
		self.hashtable = opensync.OSyncHashTable()

	def connect(self, ctx):
		rc = self.hashtable.load(self.member)
		assert(rc)
		try:
			self.comms = PhoneComms(PHONE_DEVICE) # FIXME: use config data
			self.access = PhoneAccess(self.comms)
			self.comms.open_calendar()
		except IOError, e:
			ctx.report_error(opensync.ERROR_IO_ERROR, str(e))
			return
		ctx.report_success()

	def get_changeinfo(self, ctx):
		if self.member.get_slow_sync("event"):
			self.hashtable.set_slow_sync("event")
		for change in self.access.list_changes():
			self.hashtable.detect_change(change)
			if change.changetype != opensync.CHANGE_UNMODIFIED:
				change.report(ctx)
				self.hashtable.update_hash(change)
		self.hashtable.report_deleted(ctx, "event")
		ctx.report_success()

	def commit_change(self, ctx, change):
		assert(change.objtype == "event")
		if change.changetype == opensync.CHANGE_DELETED:
			self.access.delete_event(change.uid)
		else:
			self.access.update_event(change)
		self.hashtable.update_hash(change)
		ctx.report_success()

	def disconnect(self, ctx):
		del self.access
		del self.comms
		self.hashtable.close()
		ctx.report_success()

	def sync_done(self, ctx):
		self.comms.close_calendar()
		self.hashtable.forget()
		ctx.report_success()

	def finalize(self):
		del self.hashtable

def initialize(member):
	return SyncClass(member)

def get_info(info):
	info.name = "moto-sync"
	info.accept_objtype("event")
	info.accept_objformat("event", "vevent20")

	# TODO: add support for syncing phonebook contacts
	#info.accept_objtype("contact")
	#info.accept_objformat("contact", "vcard30")

# debug code (not used when plugin is loaded by opensync)
if __name__ == "__main__" and hasattr(sys,"argv"):
	pc = PhoneComms(PHONE_DEVICE)
	pa = PhoneAccess(pc)
	pc.open_calendar()
	if len(sys.argv) > 1 and sys.argv[1] == '--delete':
		# delete events
		for pos in range(0,40):
			pc.delete_event(pos)
	else:
		for c in pa.list_changes():
			print c.data
	pc.close_calendar()