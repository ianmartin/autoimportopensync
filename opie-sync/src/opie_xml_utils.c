/* 

   Copyright 2005 Paul Eggleton

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

/*
 *  $Id: opie_xml.c,v 1.11 2004/02/20 15:55:14 irix Exp $
 */

#include "opie_xml_utils.h"

#include <glib.h>
#include <string.h>

char *xml_node_to_text(xmlDoc *doc, xmlNode *node) {
	xmlBufferPtr bufptr = xmlBufferCreate();
	xmlNodeDump(bufptr, doc, node, 0, 0);
	int length = xmlBufferLength(bufptr);
	char *nodetext = g_malloc0(length+1);
	memcpy(nodetext, xmlBufferContent(bufptr), length);
	xmlBufferFree(bufptr);
	return nodetext;
}

