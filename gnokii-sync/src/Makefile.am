gnokii_syncdir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
formatsdir=@OPENSYNC_FORMATSDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

INCLUDES = @PACKAGE_CFLAGS@

AM_CFLAGS = -Wall -Werror -O2 @PACKAGE_CFLAGS@

EXTRA_DIST = gnokii-sync gnokii_sync.h gnokii_calendar.h gnokii_calendar_utils.h gnokii_contact_utils.h gnokii_comm.h gnokii_config.h 

config_DATA = gnokii-sync

opensyncheader_HEADER = gnokii_sync.h gnokii_config.h gnokii_comm.h gnokii_calendar.h gnokii_calendar_utils.h gnokii_contact_utils.h

gnokii_sync_LTLIBRARIES = gnokii_sync.la

gnokii_sync_la_SOURCES = gnokii_sync.c gnokii_config.c gnokii_comm.c gnokii_calendar.c gnokii_contact.c
gnokii_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
gnokii_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

formats_LTLIBRARIES = gnokii.la

gnokii_la_SOURCES = gnokii_calendar_format.c gnokii_calendar_utils.c gnokii_contact_format.c gnokii_contact_utils.c gnokii_format.c
gnokii_la_LDFLAGS = -avoid-version -export-dynamic -module
gnokii_la_LIBADD = @PACKAGE_LIBS@ -lopensync-xml @LIBS@
