
#ifndef _OPENSYNC_TIME_INTERNALS_H_
#define _OPENSYNC_TIME_INTERNALS_H_

#include "opensync_xml.h"

/* Timezone ID helper */
struct tm *osync_time_dstchange(xmlNode *dstNode);
osync_bool osync_time_isdst(const char *vtime, xmlNode *tzid);
int osync_time_tzoffset(const char *vtime, xmlNode *tz);
char *osync_time_tzid(xmlNode *tz);
char *osync_time_tzlocation(xmlNode *tz);
xmlNode *osync_time_tzinfo(xmlNode *root, const char *tzid);
char *osync_time_tzlocal2utc(xmlNode *root, const char *field);

#endif // _OPENSYNC_TIME_INTERNALS_H_

