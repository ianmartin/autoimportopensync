#!/usr/bin/python
#
# Copyright (c) 2006 Eduardo Pereira Habkost <ehabkost@raisama.net>

"""Simple helper for the Google Calendar plugin

This helper should die when a proper C or C++ implementation
of a Google Calendar library is made
"""

import sys, httplib2, os, re
from xml.dom import minidom as md
from xml import xpath

# XML namespaces:
NS_GD = u'http://schemas.google.com/g/2005'
NS_OS = u'http://a9.com/-/spec/opensearchrss/1.0/'
NS_ATOM = u'http://www.w3.org/2005/Atom'

NS_DICT = {
	u'atom':NS_ATOM,
	u'os':NS_OS,
	u'gd':NS_GD
}

EVSTATUS_CANCELLED = 'http://schemas.google.com/g/2005#event.canceled'

# The escape functions are temporary (erm... this whole helper script
# is temporary  8), until file-sync supports UID generation when
# an Add is sent with an UID containing invalid characters

def escape(uri):
	def cescape(c):
		if not c.isalnum():
			return "%%%02x" % (ord(c))
		else:
			return c
	return ''.join([cescape(c) for c in uri])

def unescape(uri):
	def unescape(s):
		if s[0] == '%':
			return chr(int(s[1:3], 16)), s[3:]
		else:
			return s[0],s[1:]
	r = ''
	while uri:
		c,uri = unescape(uri)
		r += c
	return r

class DateConversionError(Exception):
	pass

DATE_RE = re.compile("([0-9][0-9][0-9][0-9])([0-9][0-9])([0-9][0-9])(T[0-9]*)?(Z?)")
TIME_RE = re.compile("T([0-9][0-9])([0-9][0-9])([0-9][0-9]|)")

def osyncToXsdate(s):
	"""Translate OpenSync date/time to xs:date or xs:dateTime value"""

	#TODO: Handle TimezoneId element

	m = DATE_RE.match(s)
	if not m:
		raise DateConversionError("Invalid date/time: %s" % (s))

	year,month,day = [int(m.group(g)) for g in 1,2,3]
	time = m.group(4)
	zulu = m.group(5)

	r = '%04d-%02d-%02d' % (year, month, day)

	if time:
		m = TIME_RE.match(time)
		if not m:
			raise DateConversionError("Invalid time: %s" % (time))
		timeparts = [m.group(g) for g in 1,2,3]
		if not timeparts[2]:
			timeparts[2] = '0'
		h,min,sec = [int(p) for p in timeparts]
		r += 'T%02d:%02d:%02d' % (h,min,sec)

	if zulu:
		r += 'Z'

	sys.stderr.write("osync to xs: %s => %s\n" % (s, r))
	return r

XSDATE_RE = re.compile("([0-9]+)-([0-9]+)-([0-9]+)(T[0-9]+:[0-9]+:[0-9]+|)(Z|[+-][0-9]+:[0-9]+|)")
XSTIME_RE = re.compile("T([0-9]+):([0-9]+):([0-9]+)")

def xsdateToOsync(s):
	#TODO: handle timezone properly
	m = XSDATE_RE.match(s)
	if not m:
		raise DateConversionError("Invalid date/time: %s" % (s))
	year,month,day = [int(m.group(g)) for g in 1,2,3]
	time = m.group(4)
	tzone = m.group(5)

	r = '%04d%02d%02d' % (year, month, day)

	if time:
		m = XSTIME_RE.match(time)
		if not m:
			raise DateConversionError("Invalid time: %s" % (time))
		h,min,sec = [int(m.group(g)) for g in 1,2,3]
		r += 'T%02d%02d%02d' % (h,min,sec)

	if tzone:
		if tzone == 'Z':
			r += 'Z'
		else:
			sys.stderr.write("Warning: Not handling timezone: %s\n" % (tzone))

	sys.stderr.write("xs to osync: %s => %s\n" % (s, r))
	return r

class GCalRequestError(Exception):
	def __init__(self, response):
		self.response = response

	def __str__(self):
		return "Http error: %02d: %s" % (self.response.status, self.response.reason)

class GCalEntry:
	def __init__(self, atom = None, osync = None):
		if not atom is None:
			self.parseAtom(atom)
		elif not osync is None:
			self.parseOsync(osync)

	def parseAtom(self, element):
		self.element = element

		self.title = self.elementValue('atom:title/text()')
		self.id = self.elementValue('atom:id/text()')
		self.editUri = self.elementValue('atom:link[@rel="edit"]/@href')
		self.content = self.elementValue('atom:content[@type="text"]/text()')
		self.dtstart = self.elementValue('gd:when/@startTime')
		self.dtend = self.elementValue('gd:when/@endTime')

		self.eventStatus = self.elementValue('gd:eventStatus/@value')

		#FIXME: Handle recurrency
		#FIXME: Handle gd:eventStatus


	def parseOsync(self, element):
		self.element = element

		self.title = self.elementValue('Event/Summary/Content/text()')
		self.content = self.elementValue('Event/Description/Content/text()')
		self.dtstart = osyncToXsdate(self.elementValue('Event/DateStarted/Content/text()'))
		self.dtend = osyncToXsdate(self.elementValue('Event/DateEnd/Content/text()'))
		#TODO: UID, editUri, dtstart, dtend, recurrency, eventStatus


	def dumpOsync(self):
		di = md.getDOMImplementation()
		doc = di.createDocument(None, 'vcal', None)
		vcal = doc.documentElement
		event = self.addElement(doc, vcal, 'Event')

		self.addElementContent(doc, event, 'Summary', self.title)
		self.addElementContent(doc, event, 'Description', self.content)

		self.addElementContent(doc, event, 'DateStarted', xsdateToOsync(self.dtstart))
		self.addElementContent(doc, event, 'DateEnd', xsdateToOsync(self.dtend))
		#TODO: UID, editUri, dtstart, dtend, recurrency, eventStatus
		return doc.toxml(encoding='utf-8')

	def dumpGdata(self):
		di = md.getDOMImplementation()
		doc = di.createDocument(None, 'entry', None)

		entry = doc.documentElement
		entry.setAttribute('xmlns', NS_ATOM)
		entry.setAttribute('xmlns:gd', NS_GD)

		title = self.addElement(doc, entry, 'title', self.title)
		title.setAttribute('type', 'text')

		content = self.addElement(doc, entry, 'content', self.content)
		content.setAttribute('type', 'text')

		when = self.addElement(doc, entry, 'gd:when')
		when.setAttribute('startTime', self.dtstart)
		when.setAttribute('endTime', self.dtend)

		return doc.toxml(encoding='utf-8')

	@staticmethod
	def addElement(doc, parent, name, text = None):
		elem = doc.createElement(name)
		parent.appendChild(elem)
		if not text is None:
			txt = doc.createTextNode(text)
			elem.appendChild(txt)
		return elem

	@staticmethod
	def addElementContent(doc, parent, name, content):
		e = GCalEntry.addElement(doc, parent, name)
		cont = GCalEntry.addElement(doc, e, 'Content', content)
		return e

	def elementValue(self, name):
		"""Return value of first element matching xpath expression"""
		nodes = self.query(name)
		if not nodes:
			return ''
		return nodes[0].nodeValue

	def query(self, expr):
		"""XPath query"""
		ctx = xpath.Context.Context(self.element, processorNss=NS_DICT)
		return xpath.Evaluate(expr, context=ctx)

class GCalHelper:
	def __init__(self, url, user, pwd):
		self.url = url
		self.h = httplib2.Http()
		self.h.follow_all_redirects = True
		self.h.add_credentials(user, pwd)
		self.hdrs = { 'Content-Type':'application/atom+xml' }

	@staticmethod
	def check_response_success(r):
		"""Check if if a httplib2 response is successful"""
		if (r.status // 100) != 2:
			raise GCalRequestError(r)

	def oper_dump(self, argv):
		r,c = self.h.request(self.url)
		self.check_response_success(r)
		sys.stdout.write(c)

	def oper_get_all(self, argv):
		r,c = self.h.request(self.url)
		self.check_response_success(r)
		doc = md.parseString(c)
		entries = doc.getElementsByTagNameNS(NS_ATOM, 'entry')
		for xe in entries:
			e = GCalEntry(atom=xe)
			if e.eventStatus == EVSTATUS_CANCELLED:
				continue

			data = e.dumpOsync()
			uid = escape(e.id)
			hash = escape(e.editUri)
			sys.stdout.write("%d %d %d\n%s%s%s" % (len(data), len(uid), len(hash), data, uid, hash))

	def oper_test(self, argv):
		doc = md.parseString(sys.stdin.read())
		entries = doc.getElementsByTagNameNS(NS_ATOM, 'entry')
		for xe in entries:
			e = GCalEntry(atom=xe)
			print e.dumpOsync()

	def oper_delete(self, argv):
		editUri = unescape(argv.pop(0))

		r,c = self.h.request(editUri, method='DELETE')
		self.check_response_success(r)

	def oper_edit_test(self, argv):
		id = argv.pop(0)

		r,c = self.h.request(id)
		self.check_response_success(r)

		xml = md.parseString(c)
		e = GCalEntry(atom=xml.documentElement)

		open('/tmp/xml.txt', 'w').write(c)
		os.system('vi /tmp/xml.txt')
		c = open('/tmp/xml.txt', 'r').read()

		self.oper_edit([escape(e.editUri)], c)

	def oper_edit(self, argv, data = None):
		editUri = unescape(argv.pop(0))

		#r,c = self.h.request(self.url)
		#self.check_response_success(r)

		if data is None:
			entryData = sys.stdin.read()
			xml = md.parseString(entryData)
			e = GCalEntry(osync=xml.documentElement)
			gdata = e.dumpGdata()
		else:
			gdata = data

		#r,c = self.h.request(id)
		#self.check_response_success(r)
		#gdata = c

		sys.stderr.write("edit gdata: %s\n" % (gdata))

		r,c = self.h.request(editUri, method='PUT', body=gdata, headers=self.hdrs)
		self.check_response_success(r)

		sys.stderr.write("response: %s\n" % c)

		xml = md.parseString(c)
		e = GCalEntry(atom=xml.documentElement)
		
		data = e.dumpOsync()
		uid = escape(e.id)
		hash = escape(e.editUri)
		sys.stdout.write("%d %d %d\n%s%s%s" % (len(data), len(uid), len(hash), data, uid, hash))

	def oper_add(self, argv):
		entryData = sys.stdin.read()
		xml = md.parseString(entryData)
		e = GCalEntry(osync=xml.documentElement)
		gdata = e.dumpGdata()

		sys.stderr.write("add gdata: %s\n" % (gdata))


		r,c= self.h.request(self.url, method='POST', body=gdata, headers=self.hdrs)
		self.check_response_success(r)

		xml = md.parseString(c)
		e = GCalEntry(atom=xml.documentElement)
		
		data = e.dumpOsync()
		uid = escape(e.id)
		hash = escape(e.editUri)
		sys.stdout.write("%d %d %d\n%s%s%s" % (len(data), len(uid), len(hash), data, uid, hash))
		

def main(argv):
	me = argv.pop(0)
	url = argv.pop(0)
	user = argv.pop(0)
	passfd = int(argv.pop(0))
	operation = argv.pop(0)

	pwd = os.fdopen(passfd).readline()

	h = GCalHelper(url, user, pwd)
	fn = getattr(h, 'oper_%s' % (operation))
	try:
		return fn(argv)
	except GCalRequestError,re:
		sys.stderr.write("%s\n" % (re))
		sys.stderr.write("%s\n" % (repr(re.response)))
		return 1


if __name__ == '__main__':
	sys.exit(main(sys.argv))
