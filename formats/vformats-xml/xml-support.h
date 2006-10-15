#ifndef _XML_SUPPORT_H
#define _XML_SUPPORT_H

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

#include <opensync/opensync.h>
#include <opensync/opensync_xml.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif
	
typedef struct OSyncXMLScore {
	int value;
	const char *path;
} OSyncXMLScore;

OSyncConvCmpResult osxml_compare(xmlDoc *leftinpdoc, xmlDoc *rightinpdoc, OSyncXMLScore *scores, int default_score, int treshold);

#ifdef __cplusplus
}
#endif

#endif // _XML_SUPPORT_H
