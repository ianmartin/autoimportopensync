/*
 * tomboy_note - convert tomboy notes to xmlformat-note and backwards
 * Copyright (C) 2008  Bjoern Ricks <bjoern.ricks@gmail.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */

#include "tomboy_note.h"
#include "tomboy_note_internal.h"

osync_bool tomboynote_validate(xmlDocPtr doc) {
	osync_assert(doc);
	osync_trace(TRACE_ENTRY, "%s (%p)", __func__, doc);
	
	xmlNodePtr rootnode;
	rootnode = xmlDocGetRootElement(doc);
	
	if (rootnode == NULL) {
		return FALSE;
	}
	
	if ( xmlStrEqual(rootnode->name, BAD_CAST "note")  ) { //TODO: use xml schema validation
		osync_trace(TRACE_EXIT, "%s", __func__);
		return TRUE;
	}
	osync_trace(TRACE_EXIT, "%s", __func__);	
	return FALSE;
}

GList * tomboynote_parse_tags(xmlDocPtr doc) {
	osync_assert(doc);
	
	xmlNodePtr tag;
	xmlNodePtr cur;
	
	GList* list = NULL;
	
	for ( cur = xmlDocGetRootElement(doc)->children; cur != NULL; cur = cur->next ) {
		if ( xmlStrEqual(cur->name, BAD_CAST "tags") ) { // <tags> found
			for( tag = cur->children; tag != NULL; tag = tag->next ) {
				if ( xmlStrEqual(tag->name, BAD_CAST "tag") && tag->children != NULL ) {
					list = g_list_append (list, tag->children->content);
				}
			}
			return list;
		}
	}

	return list;
}

const char * tomboynote_parse_node(xmlDocPtr doc, const char * nodename) {
	osync_assert(doc);
	osync_assert(nodename);
	
	xmlNodePtr cur;
	for (cur = xmlDocGetRootElement(doc)->children; cur != NULL; cur = cur->next) {
		if ( xmlStrEqual(cur->name, BAD_CAST nodename) && cur->children != NULL ) { // <nodename> found
			return (const char *)cur->children->content; 
		}
	}
	return NULL;
}

void tomboynote_parse_content_node(xmlNodePtr node, GString * output) {
	osync_assert(node);
	osync_assert(output)

	xmlNodePtr cur;
	if ( node != NULL ) {
		for (cur = node; cur != NULL; cur = cur->next ) {
			if ( cur->type == XML_TEXT_NODE ) {
				output = g_string_append(output, (const char *)cur->content);
			}
			else if ( cur->type == XML_ELEMENT_NODE ) {
			// add format highlights
				if ( xmlStrEqual(cur->name, BAD_CAST "strikethrough") ) {
					//TODO add plain strike format
					tomboynote_parse_content_node(cur->children, output);
					}
				else if ( xmlStrEqual(cur->name, BAD_CAST "highlight") ) {
					//TODO add plain highlight format
					tomboynote_parse_content_node(cur->children, output);
				}
					else if ( xmlStrEqual(cur->name, BAD_CAST "bold") ) {
						//TODO add plain bold format
						tomboynote_parse_content_node(cur->children, output);
					}
					else if ( xmlStrEqual(cur->name, BAD_CAST "italic") ) {
					    //TODO add plain italic format
						tomboynote_parse_content_node(cur->children, output);
					}
					else if ( xmlStrEqual(cur->name, BAD_CAST "monospace") ) {
					    //TODO add plain monospace format
						tomboynote_parse_content_node(cur->children, output);
					}
					// add link declaration
					else if ( xmlStrEqual(cur->ns->href, BAD_CAST "http://beatniksoftware.com/tomboy/link") ) {
						tomboynote_parse_content_node(cur->children, output);
					}
					// add size decleration
					else if ( xmlStrEqual(cur->ns->href, BAD_CAST "http://beatniksoftware.com/tomboy/size") ) {
						tomboynote_parse_content_node(cur->children, output);
					}
					// add listitems
					else if ( xmlStrEqual(cur->name, BAD_CAST "list") ) {
						xmlNodePtr listitem;
						xmlAttrPtr attribute;
						for ( listitem = cur->children; listitem != NULL; listitem = listitem->next ) {
							if ( xmlStrEqual(listitem->name , BAD_CAST "list-item" ) ) {
								attribute = listitem->properties; //TODO iterate through attributes
								if ( xmlStrEqual(attribute->name, BAD_CAST "dir") ) {
									output = g_string_append(output, "* ");	
								}
								tomboynote_parse_content_node(listitem->children, output);
								output = g_string_append(output, "\n");
							}
						}
					}
				}
		}
	}
}

void tomboynote_parse_content(xmlDocPtr doc, GString * output) {
	xmlNodePtr cur;
	xmlXPathContextPtr xpathCtx; 
	xmlXPathObjectPtr xpathObj;
	xmlNodeSetPtr nodes;
	int size;

	osync_trace(TRACE_ENTRY, "%s (%p,%p)", __func__, doc, output);
	
	xpathCtx = xmlXPathNewContext(doc);
	if (xpathCtx == NULL) {
		osync_trace(TRACE_EXIT, "%s", __func__);
		return;
	}
	if( xmlXPathRegisterNs(xpathCtx, BAD_CAST "tomboy", BAD_CAST "http://beatniksoftware.com/tomboy") != 0 ) {
		xmlXPathFreeContext(xpathCtx);
		osync_trace(TRACE_EXIT, "%s", __func__);
		return;
	}
	xpathObj = xmlXPathEvalExpression(BAD_CAST "/tomboy:note/tomboy:text/tomboy:note-content/node()", xpathCtx);
	if ( xpathObj != NULL ) {		
		nodes = xpathObj->nodesetval;
		size = (nodes) ? nodes->nodeNr : 0;
		if(size > 0) {
			osync_trace(TRACE_INTERNAL, "parsing tomboy note content" );
			cur = nodes->nodeTab[0];
			tomboynote_parse_content_node(cur, output);
		}
	}
	xmlXPathFreeContext(xpathCtx);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

// converter functions
osync_bool conv_tomboynote_to_xmlformat(char *data, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error) {
	osync_trace(TRACE_ENTRY, "start:%s", __func__);
	
	xmlDocPtr doc;
	xmlParserCtxtPtr ctxt; 

	OSyncXMLField * xmlfield;

	const char * node_data;

	GString *str;
	GList * list_tags;
	GList * list_tag;

	if (!data) {
		return FALSE;
	}
	
	ctxt = xmlNewParserCtxt();
	if ( ctxt == NULL ) {
		return FALSE;
	}
	doc = xmlCtxtReadMemory(ctxt,data,inpsize,NULL,NULL,0);
	if ( doc == NULL ) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to parse xml doc tree.");
		goto FREE_CONTEXT;
	}

	str = g_string_new("");
	
	OSyncXMLFormat * xmlformat = osync_xmlformat_new("note", error);
	
	// parse tomboy tags as Categories
	list_tags = tomboynote_parse_tags(doc);
	if ( list_tags != NULL ) {
		xmlfield = osync_xmlfield_new(xmlformat, "Categories", error);
		for ( list_tag = g_list_first(list_tags); list_tag != NULL; list_tag = list_tag->next ) {
			osync_xmlfield_add_key_value(xmlfield, "Category", list_tag->data);
		}
	}
	// parse create-date as created
	node_data = tomboynote_parse_node(doc, "create-date");
	if ( node_data != NULL ) {
		xmlfield = osync_xmlfield_new(xmlformat, "Created", error);
		osync_xmlfield_set_key_value(xmlfield, "Content", node_data);
		osync_xmlfield_set_attr(xmlfield, "Value", "DATE-TIME");
		osync_xmlfield_set_attr(xmlfield, "TimezoneID", "UTC" ); //TODO get timezone info
	}
	
	// parse content as description
	tomboynote_parse_content(doc, str);
	xmlfield = osync_xmlfield_new(xmlformat, "Description", error);
	osync_xmlfield_set_key_value(xmlfield, "Content", str->str);

	//parse last-change-date as lastmodified
	node_data = tomboynote_parse_node(doc, "last-change-date");
	if ( node_data != NULL ) {
		xmlfield = osync_xmlfield_new(xmlformat, "LastModified", error);
		osync_xmlfield_set_key_value(xmlfield, "Content", node_data);
		osync_xmlfield_set_attr(xmlfield, "Value", "DATE-TIME");
		osync_xmlfield_set_attr(xmlfield, "TimezoneID", "UTC" ); //TODO get timezone info
	}
	
	//parse title as summary
	node_data = tomboynote_parse_node(doc, "title");
	if ( node_data != NULL ) {
		xmlfield = osync_xmlfield_new(xmlformat, "Summary", error);
		osync_xmlfield_set_key_value(xmlfield, "Content", node_data);
	}
	
	*free_input = TRUE;
	*output = (char *)xmlformat;
	*outpsize = sizeof(xmlformat);
	// debug output
	unsigned int size;
	char *cstr;
	osync_xmlformat_assemble(xmlformat, &cstr, &size);
	osync_trace(TRACE_SENSITIVE, "... Output XMLFormat is: \n%s", cstr);
	g_free(cstr);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
	
	xmlFreeDoc(doc);
FREE_CONTEXT:
	xmlFreeParserCtxt(ctxt);
	osync_trace(TRACE_EXIT_ERROR, "%s", __func__);
	return FALSE;	
}

osync_bool conv_xmlformat_to_tomboynote(char *input, unsigned int inpsize, char **output, unsigned int *outpsize, osync_bool *free_input, const char *config, OSyncError **error)
{
	xmlNsPtr ns;
	xmlDocPtr doc;
	xmlNodePtr note;
	xmlNodePtr title;
	xmlNodePtr text;
	xmlNodePtr content;
	xmlNodePtr last_change_date;
	xmlNodePtr create_date;
	xmlNodePtr tags;
	xmlNodePtr tag;
	xmlNodePtr tmp_node;
	xmlAttrPtr version;
	xmlAttrPtr xml_preserve;
	
	const char * content_text;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p, %p, %p, %p)", __func__, input, inpsize, output, outpsize, free_input, config, error);
	
	doc = xmlNewDoc(BAD_CAST "1.0");
	ns = xmlNewNs(NULL, NULL, BAD_CAST "http://beatniksoftware.com/tomboy");
	
	note = xmlNewNode(ns, BAD_CAST "note");
	version = xmlNewProp(note, BAD_CAST "version", BAD_CAST "0.3" );
	xmlDocSetRootElement(doc, note);
	title = xmlNewNode(ns, BAD_CAST "title" );
	xmlAddChild(note, title);
	text = xmlNewNode(ns, BAD_CAST "text");
	xml_preserve = xmlNewProp(text, BAD_CAST "xml:space", BAD_CAST "preserve" );
	xmlAddChild(note, text);
	content = xmlNewNode(ns, BAD_CAST "note-content" );
	version = xmlNewProp(content, BAD_CAST "version", BAD_CAST "0.1");
	xmlAddChild(text, content);
	tags = xmlNewNode(ns, BAD_CAST "tags");
	create_date = xmlNewNode(ns, BAD_CAST "create-date");
	last_change_date = xmlNewNode(ns, BAD_CAST "last-change-date");
	
	// Print input XMLFormat into terminal
	OSyncXMLFormat *xmlformat = (OSyncXMLFormat *)input;
	unsigned int size;
	char *str;
	osync_xmlformat_assemble(xmlformat, &str, &size);
	osync_trace(TRACE_SENSITIVE, "Input XMLFormat is:\n%s", str);
	g_free(str);
	
	OSyncXMLField *xmlfield = osync_xmlformat_get_first_field(xmlformat);
	for(; xmlfield != NULL; xmlfield = osync_xmlfield_get_next(xmlfield)) {
		if ( !strcmp(osync_xmlfield_get_name(xmlfield),"Categories") ) {
			content_text = osync_xmlfield_get_key_value(xmlfield, "Category");
			tag = xmlNewNode(ns, BAD_CAST "tag" );
			xmlNodeSetContent(tag, BAD_CAST content_text);
			xmlAddChild(tags, tag);
		}
		else if ( !strcmp(osync_xmlfield_get_name(xmlfield),"Created") ) {
			content_text = osync_xmlfield_get_key_value(xmlfield, "Content");
			xmlNodeSetContent(create_date, BAD_CAST content_text);
		}
		else if ( !strcmp(osync_xmlfield_get_name(xmlfield),"Description") ) {
			content_text = osync_xmlfield_get_key_value(xmlfield, "Content");
			xmlNodeSetContent(content, BAD_CAST content_text);
		}
		else if ( !strcmp(osync_xmlfield_get_name(xmlfield),"LastModified") ) {
			content_text = osync_xmlfield_get_key_value(xmlfield, "Content");
			xmlNodeSetContent(last_change_date, BAD_CAST content_text);
		}
		else if ( !strcmp(osync_xmlfield_get_name(xmlfield),"Summary") ) {
			content_text = osync_xmlfield_get_key_value(xmlfield, "Content");
			xmlNodeSetContent(title, BAD_CAST content_text);
		}
	}
	// add <last-change-date>
	xmlAddChild(note, last_change_date);
	// add <last-metadata-change-date> 
	// don't know the difference between last-change-date and last-metadata-change-date
	// it seems that always both are changed
	tmp_node = xmlNewNode(ns, BAD_CAST "last-metadata-change-date" ); 
	xmlNodeSetContent(tmp_node, xmlStrdup(last_change_date->content));
	xmlAddChild(note, tmp_node);
	// add <create-date>
	xmlAddChild(note, create_date); 
	// add <cursor-position>
	tmp_node = xmlNewNode(ns, BAD_CAST "cursor-position" ); 
	xmlNodeSetContent(tmp_node, BAD_CAST "1"); // first position
	xmlAddChild(note, tmp_node);
	// add <width>
	tmp_node = xmlNewNode(ns, BAD_CAST "width" );
	xmlNodeSetContent(tmp_node, BAD_CAST "300");
	xmlAddChild(note, tmp_node);
	// add <height>
	tmp_node = xmlNewNode(ns, BAD_CAST "height" );
	xmlNodeSetContent(tmp_node, BAD_CAST "400");
	xmlAddChild(note, tmp_node);
	// add <x>
	tmp_node = xmlNewNode(ns, BAD_CAST "x" );
	xmlNodeSetContent(tmp_node, BAD_CAST "100");
	xmlAddChild(note, tmp_node);
	// add <y>
	tmp_node = xmlNewNode(ns, BAD_CAST "y" );
	xmlNodeSetContent(tmp_node, BAD_CAST "100");
	xmlAddChild(note, tmp_node);
	// add <tags>
	if ( tags->children != NULL ) {
		xmlAddChild(note, tags);
	}
	// add <open-on-startup>
	tmp_node = xmlNewNode(ns, BAD_CAST "open-on-startup" );
	xmlNodeSetContent(tmp_node, BAD_CAST "FALSE");
	xmlAddChild(note, tmp_node);
	
	*free_input = TRUE;
	xmlDocDumpFormatMemory(doc, (xmlChar **)output, (int *)outpsize, 1);
	if (!*output) {
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to create tomboy-note", __func__);
		return FALSE;
	}
			
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

osync_bool detect_tomboynote(const char *data, int size) {
	osync_trace(TRACE_ENTRY, "start:%s (%p,%d)", __func__, data, size);
	
	xmlDocPtr doc;
	xmlParserCtxtPtr ctxt;
	
	if (!data) {
		return FALSE;
	}
	/* TODO change xml check
	 * The g_pattern_match* functions match a string against a pattern containing '*' and '?' wildcards with similar semantics as the standard glob() function: '*' matches an arbitrary, possibly empty, string, '?' matches an arbitrary character.
	 * Note that in contrast to glob(), the '/' character can be matched by the wildcards, there are no '[...]' character ranges and '*' and '?' can not be escaped to include them literally in a pattern. 
	 */
	if (!g_pattern_match_simple("?<?xml version=\"1.0\"*?>*", data)) {
		osync_trace(TRACE_EXIT, "%s not xml data", __func__);
		return FALSE;
	}
	
	ctxt = xmlNewParserCtxt();
	if (ctxt == NULL) {
		osync_trace(TRACE_EXIT, "%s", __func__);
		return FALSE;
	}
	doc = xmlCtxtReadMemory(ctxt,data,size,NULL,NULL,0);
	if (doc == NULL) {
		osync_trace(TRACE_EXIT, "%s", __func__);
		goto FREE_CONTEXT;
	}
	
	if ( tomboynote_validate(doc) ) {
		osync_trace(TRACE_EXIT, "%s valid tomboy-note", __func__);
		return TRUE;
	}
	
	xmlFreeDoc(doc);
FREE_CONTEXT:
	xmlFreeParserCtxt(ctxt);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return FALSE;
}

// format functions

static void destroy_tomboynote(char *input, unsigned int inpsize)
{
	free(input);
}

static void create_tomboynote(char **data, unsigned int *size) {
	xmlNsPtr ns;
	xmlDocPtr doc;
	xmlNodePtr node;
	xmlAttrPtr version;
	
	doc = xmlNewDoc(BAD_CAST "1.0");
	ns = xmlNewNs(NULL, NULL, BAD_CAST "http://beatniksoftware.com/tomboy");
	
	node = xmlNewNode(ns, BAD_CAST "note");
	version = xmlNewProp(node, BAD_CAST "version", BAD_CAST "0.3" );
	xmlDocSetRootElement(doc, node);
	
	xmlDocDumpFormatMemory(doc, (xmlChar **)data, (int *)size, 1);
	if (!*data) {
		osync_trace(TRACE_ERROR, "%s: Unable to create tomboy-note %s", __func__);
	}
	xmlFreeDoc(doc);
	xmlFreeNode(node);
	xmlFreeNs(ns);
}

static OSyncConvCmpResult compare_tomboynote(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize) {
	//TODO
	return OSYNC_CONV_DATA_MISMATCH;
}

osync_bool get_format_info(OSyncFormatEnv *env, OSyncError **error)
{
	OSyncObjFormat *format = osync_objformat_new("tomboy-note", "note", error);
	if (!format)
		return FALSE;
	
	osync_objformat_set_create_func(format, create_tomboynote);
	osync_objformat_set_destroy_func(format, destroy_tomboynote);
	
/*	osync_objformat_set_compare_func(format, compare_tomboynote);
	osync_objformat_set_duplicate_func(format, duplicate_xmlformat);
	osync_objformat_set_print_func(format, print_xmlformat);
	osync_objformat_set_copy_func(format, copy_xmlformat);

	osync_objformat_set_revision_func(format, get_note_revision);

	osync_objformat_must_marshal(format);
	osync_objformat_set_marshal_func(format, marshal_xmlformat);
	osync_objformat_set_demarshal_func(format, demarshal_xmlformat);
	*/
	
	osync_format_env_register_objformat(env, format);
	osync_objformat_unref(format);

	return TRUE;
}

osync_bool get_conversion_info(OSyncFormatEnv *env, OSyncError **error)
{
	OSyncObjFormat *plain = osync_format_env_find_objformat(env, "plain");
	if (!plain) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find plain format");
		return FALSE;
	}
	
	OSyncObjFormat *tomboynote = osync_format_env_find_objformat(env, "tomboy-note");
	if (!tomboynote) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find tomboy-note format");
		return FALSE;
	}
	
	OSyncFormatConverter *conv = osync_converter_new_detector(plain, tomboynote, detect_tomboynote, error);
	if (!conv)
		return FALSE;
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	OSyncObjFormat *xmlformat = osync_format_env_find_objformat(env, "xmlformat-note");
	if (!xmlformat) {
		/* This is the first error that occurs if /usr/lib/opensync-1.0/formats 
		 is not populated correctly.  So report it! */
		osync_trace(TRACE_ERROR, "Unable to find object format xmlformat-note");
		return FALSE;
	}
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, tomboynote, conv_xmlformat_to_tomboynote, error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(error));
		osync_error_unref(error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, tomboynote, xmlformat, conv_tomboynote_to_xmlformat, error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(error));
		osync_error_unref(error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	return TRUE;
}
	
int get_version (void)
{
	return TOMBOY_FORMAT_OPENSYNC_PLUGINVERSION;
}
