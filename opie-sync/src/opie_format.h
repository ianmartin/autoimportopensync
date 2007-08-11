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

void xml_node_to_attr(xmlNode *node_from, const char *nodename, xmlNode *node_to, const char *attrname);
void xmlfield_key_to_attr(OSyncXMLField *xmlfield, const char *key, xmlNode *node_to, const char *attrname);
time_t xml_node_vtime_to_attr_time_t(xmlNode *node_from, xmlNode *node_to, const char *attrname);
void xml_categories_to_attr(xmlNode *item_node, xmlNode *node_to, const char *category_attr);
void xmlfield_categories_to_attr(OSyncXMLField *in_xmlfield, xmlNode *node_to, const char *category_attr);
void xml_recur_attr_to_node(xmlNode *item_node, xmlNode *node_to, GDate *startdate);
void xml_recur_node_to_attr(xmlNode *item_node, xmlNode *node_to);
void xml_todo_alarm_attr_to_node(const char *alarmstr, xmlNode *node_to, time_t *starttime);
void xml_todo_alarm_node_to_attr(xmlNode *item_node, xmlNode *node_to);
void xml_cal_alarm_node_to_attr(xmlNode *item_node, xmlNode *node_to, time_t *starttime);

#endif /* _OPIE_FORMAT_H */




