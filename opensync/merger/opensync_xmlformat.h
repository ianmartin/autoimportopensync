#ifndef OPENSYNC_XMLFORMAT_H_
#define OPENSYNC_XMLFORMAT_H_

#include "opensync/format/opensync_objformat.h"

typedef struct OSyncXMLPoints {
	char *fieldname;
	int points;
	char** keys;
} OSyncXMLPoints;


OSYNC_EXPORT OSyncXMLFormat *osync_xmlformat_new(const char *objtype);
OSYNC_EXPORT OSyncXMLFormat *osync_xmlformat_parse(const char *buffer, unsigned int size, OSyncError **error);
OSYNC_EXPORT void osync_xmlformat_ref(OSyncXMLFormat *xmlformat);
OSYNC_EXPORT void osync_xmlformat_unref(OSyncXMLFormat *xmlformat);

OSYNC_EXPORT const char *osync_xmlformat_get_objtype(OSyncXMLFormat *xmlformat);

OSYNC_EXPORT OSyncXMLField *osync_xmlformat_get_first_field(OSyncXMLFormat *xmlformat);
OSYNC_EXPORT OSyncXMLFieldList *osync_xmlformat_search_field(OSyncXMLFormat *xmlformat, const char *name, ...);

OSYNC_EXPORT osync_bool osync_xmlformat_assemble(OSyncXMLFormat *xmlformat, char **buffer, int *size);
OSYNC_EXPORT osync_bool osync_xmlformat_validate(OSyncXMLFormat *xmlformat);

OSYNC_EXPORT void osync_xmlformat_sort(OSyncXMLFormat *xmlformat);
OSYNC_EXPORT void osync_xmlformat_merging(OSyncXMLFormat *xmlfield, OSyncCapabilities *capabilities, OSyncXMLFormat *original);

OSYNC_EXPORT OSyncConvCmpResult osync_xmlformat_compare(OSyncXMLFormat *xmlformat1, OSyncXMLFormat *xmlformat2, OSyncXMLPoints points[], int basic_points, int treshold);

#endif /*OPENSYNC_XMLFORMAT_H_*/
