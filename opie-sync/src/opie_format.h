/*

   Copyright 2005 Paul Eggleton & Holger Hans Peter Freyther

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


#ifndef _OPIE_FORMAT_H
#define _OPIE_FORMAT_H

void xmlfield_key_to_attr(OSyncXMLField *xmlfield, const char *key, xmlNode *node_to, const char *attrname);
void xmlfield_uid_to_attr(OSyncXMLField *xmlfield, xmlNode *node_to);
void xml_uid_attr_to_xmlfield(const char *uid, const char *nodename, OSyncXMLFormat *out_xmlformat, OSyncError **error);
time_t xmlfield_vtime_to_attr_time_t(OSyncXMLField *xmlfield, xmlNode *node_to, const char *attrname);
OSyncXMLField *xml_attrs_to_xmlfield_keys(xmlNode *node, OSyncXMLFormat *out_xmlformat, const char *fieldname, GSList *attrs, GSList *keys, OSyncError **error);
void dual_list_append(GSList **list1, void *item1, GSList **list2, void *item2);
void dual_list_clear(GSList **list1, GSList **list2);
void xmlfield_categories_to_attr(OSyncXMLField *in_xmlfield, xmlNode *node_to, const char *category_attr);
void xml_recur_attr_to_xmlfield(xmlNode *item_node, OSyncXMLFormat *out_xmlformat, GDate *startdate, OSyncError **error);
void xmlfield_recur_to_attr(OSyncXMLField *in_xmlfield, xmlNode *node_to);
void xml_todo_alarm_attr_to_xmlfield(const char *alarmstr, OSyncXMLFormat *out_xmlformat, time_t *starttime, OSyncError **error);
void xmlformat_todo_alarms_to_attr(OSyncXMLFormat *in_xmlformat, xmlNode *node_to, const char *duedate);
void xmlformat_cal_alarms_to_attr(OSyncXMLFormat *in_xmlformat, xmlNode *node_to, time_t *starttime);

#endif /* _OPIE_FORMAT_H */

