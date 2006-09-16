#!/usr/bin/env python2.4

"""
Test/utility code for moto-sync plugin, independent of opensync
"""

import sys, types, os.path, popen2
from optparse import OptionParser

try:
    import motosync
except ImportError:
    # motosync wasn't in our standard import path
    # try looking in the opensync python plugin dir for it
    child = popen2.Popen3('pkg-config opensync-1.0 --variable=prefix')
    libdir = child.fromchild.readline().rstrip('\n')
    if child.wait() != 0 or not os.path.isdir(libdir):
        sys.stderr.write("Error: couldn't locate OpenSync library directory\n")
        sys.exit(1)
    sys.path.append(os.path.join(libdir, 'opensync', 'python-plugins'))
    import motosync

DEFAULT_DEVICE = '/dev/rfcomm0'

def parse_args():
    p = OptionParser(version=motosync.__revision__,
                     description='moto-sync test utility')
    p.add_option('-d', '--device', dest='device',
                 help='device to access phone, defaults to %s' % DEFAULT_DEVICE)
    p.add_option('-v', '--verbose', action='store_true', dest='verbose',
                 help='log phone commands to stdout')
    p.add_option('-t', '--type', action='append', dest='objtype',
                 choices=motosync.SUPPORTED_OBJTYPES,
                 help='object type to access (defaults to all types)')
    p.add_option('-f', '--file', dest='filename',
                 help='name of backup/restore data file')
    p.add_option('--list', action='store_const', dest='mode', const='list',
                 help='list data on the phone in OpenSync XML format')
    p.add_option('--backup', action='store_const', dest='mode', const='backup',
                 help='backup entries from the phone to a file')
    p.add_option('--restore', action='store_const', dest='mode', const='restore',
                 help='restore a backup created with the --backup option')
    p.add_option('--delete', action='store_const', dest='mode', const='delete',
                 help='delete all entries on the phone')
    p.set_defaults(device=DEFAULT_DEVICE, verbose=False, filename=None,
                   mode=None, objtype=None)
    options, args = p.parse_args()
    if not options.mode:
        p.error('one of the list, backup, restore, delete actions is required')
    if options.mode in ['backup', 'restore'] and not options.filename:
        p.error('this action requires a --file argument')
    return options

def prompt_user(options):
    """Prompt the user if they are trying to write to the phone."""
    if options.mode == 'delete' or options.mode == 'restore':
        if not motosync.WRITE_ENABLED:
            print 'WARNING: writes disabled, this will not save/delete anything'
        else:
            print ('WARNING: About to %s all %s entries from the phone!'
                % (options.mode, ' & '.join(options.objtype)))
        print 'Are you sure? [yn] ',
        if sys.stdin.read(1).lower() != 'y':
            print 'Operation aborted'
            return

def pack_backup(typestr, edata):
    """pack event data into a single-line string to write to the backup file"""
    strings = []
    for val in edata:
        if type(val) == types.IntType:
            strings.append(str(val))
        else:
            strings.append('"%s"' % val)
            assert('"' not in val)
    return ','.join([typestr] + strings) + '\n'

def unpack_backup(line):
    """reverse the pack_backup function above"""
    if line[-1] == '\n':
        line = line[:-1]
    parts = []
    nextpart = ''
    wasquote = False
    inquote = False
    for c in line:
        if c == ',' and not inquote:
            if not wasquote and len(parts) > 0:
                nextpart = int(nextpart)
            parts.append(nextpart)
            nextpart = ''
            wasquote = False
        elif c == '"':
            wasquote = True
            inquote = not inquote
        else:
            nextpart = nextpart + c
    if not wasquote:
        nextpart = int(nextpart)
    parts.append(nextpart)
    return parts[0], parts[1:]

def main():
    options = parse_args()
    if not options.objtype:
        options.objtype = motosync.SUPPORTED_OBJTYPES
    motosync.DEBUG_OUTPUT = options.verbose

    prompt_user(options)
    pc = motosync.PhoneComms(options.device)

    if options.mode == 'list':
        pa = motosync.PhoneAccess(pc)
        for objtype in options.objtype:
            for change in pa.list_changes(objtype):
                print change.uid
                print change.data

    if options.mode == 'delete' or options.mode == 'restore':
        if 'event' in options.objtype:
            for edata in pc.read_events():
                pc.delete_event(edata[0])
        if 'contact' in options.objtype:
            pc.read_contact_params()
            for edata in pc.read_contacts():
                pc.delete_contact(edata[0])

    if options.mode == 'backup':
        f = open(options.filename, 'w')
        if 'event' in options.objtype:
            for edata in pc.read_events():
                f.write(pack_backup('E', edata[:-1]))
                if edata[-1] != []: # exceptions
                    f.write(pack_backup('X', edata[-1]))
        if 'contact' in options.objtype:
            pc.read_contact_params()
            for edata in pc.read_contacts():
                f.write(pack_backup('C', edata))
        f.close()

    if options.mode == 'restore':
        events = []
        contacts = []
        f = open(options.filename, 'r')
        for line in f.readlines():
            typestr, edata = unpack_backup(line)
            if typestr == 'E':
                events.append(edata + [[]])
            elif typestr == 'X':
                events[-1][-1] = edata
            elif typestr == 'C':
                contacts.append(edata)
            else:
                assert(False, 'Unexpected type %s' % typestr)
        f.close()

        if 'event' in options.objtype:
            for e in events:
                pc.write_event(e)
        if 'contact' in options.objtype:
            for e in contacts:
                pc.write_contact(e)


if __name__ == "__main__":
    main()
