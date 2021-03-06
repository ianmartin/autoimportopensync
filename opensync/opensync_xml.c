/*
 * xml - A plugin for xml objects for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
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
 
#include "opensync_xml.h"
#include "opensync_internals.h"
#include <opensync/opensync-serializer.h>

void osync_xml_free(void *ptr)
{
  xmlFree(ptr);
}

void osync_xml_free_doc(xmlDoc *doc)
{
  xmlFreeDoc(doc);
}
   
xmlNode *osync_xml_node_add_root(xmlDoc *doc, const char *name)
{
  doc->children = xmlNewDocNode(doc, NULL, (xmlChar*)name, NULL);
  return doc->children;
}

xmlNode *osync_xml_node_get_root(xmlDoc *doc, const char *name, OSyncError **error)
{
  xmlNode *cur = xmlDocGetRootElement(doc);
  if (!cur) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
    return NULL;
  }
	
  if (xmlStrcmp((cur)->name, (const xmlChar *) name)) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xml root element");
    return NULL;
  }
	
  cur = (cur)->xmlChildrenNode;
  return cur;
}

void osync_xml_node_set(xmlNode *node, const char *name, const char *data, OSyncXMLEncoding encoding)
{
  if (name)
    xmlNodeSetName(node, (xmlChar*)name); //FIXME Free previous name?
		
  if (data)
    xmlNewTextChild(node, NULL, (xmlChar*)"Content", (xmlChar*)data);
}

xmlNode *osync_xml_node_add(xmlNode *parent, const char *name, const char *data)
{
  xmlNode *node = NULL;
  if (!data)
    return NULL;
  if (strlen(data) == 0)
    return NULL;
  node = xmlNewTextChild(parent, NULL, (const xmlChar*)name, (const xmlChar*)data);
  return node;
}

void osync_xml_node_add_property(xmlNode *parent, const char *name, const char *data)
{
  xmlNewProp(parent, (xmlChar*)name, (xmlChar*)data);
}

void osync_xml_node_mark_unknown(xmlNode *parent)
{
  if (!xmlHasProp(parent, (xmlChar*)"Type"))
    xmlNewProp(parent, (xmlChar*)"Type", (xmlChar*)"Unknown");
}

void osync_xml_node_remove_unknown_mark(xmlNode *node)
{
  xmlAttr *attr = xmlHasProp(node, (xmlChar*)"Type");
  if (!attr)
    return;
  xmlRemoveProp(attr);
}

xmlNode *osync_xml_get_node(xmlNode *parent, const char *name)
{
  xmlNode *cur = (parent)->xmlChildrenNode;
  while (cur) {
    if (!xmlStrcmp(cur->name, (const xmlChar *)name))
      return cur;
    cur = cur->next;
  }
  return NULL;
}

// caller is responsible for freeing, with xmlFree()
xmlChar *osync_xml_find_node(xmlNode *parent, const char *name)
{
  return xmlNodeGetContent(osync_xml_get_node(parent, name));
}

xmlXPathObject *osync_xml_get_nodeset(xmlDoc *doc, const char *expression)
{
  xmlXPathContext *xpathCtx = NULL;
  xmlXPathObject *xpathObj = NULL;
    
  /* Create xpath evaluation context */
  xpathCtx = xmlXPathNewContext(doc);
  if(xpathCtx == NULL) {
    fprintf(stderr,"Error: unable to create new XPath context\n");
    return NULL;
  }
    
  /* Evaluate xpath expression */
  xpathObj = xmlXPathEvalExpression((xmlChar*)expression, xpathCtx);
  if(xpathObj == NULL) {
    fprintf(stderr,"Error: unable to evaluate xpath expression \"%s\"\n", expression);
    xmlXPathFreeContext(xpathCtx); 
    return NULL;
  }

  xmlXPathFreeContext(xpathCtx);
  /* Cleanup of XPath data */
  // xmlXPathFreeObject(xpathObj);
  return xpathObj;
}

xmlXPathObject *osync_xml_get_unknown_nodes(xmlDoc *doc)
{
  return osync_xml_get_nodeset(doc, "/*/*[@Type='Unknown']");
}

void osync_xml_map_unknown_param(xmlNode *node, const char *paramname, const char *newname)
{
  xmlNode *cur = node->xmlChildrenNode;
  while (cur) {
    xmlChar *name = NULL;
    xmlChar *content = NULL;
    if (xmlStrcmp(cur->name, (const xmlChar *)"UnknownParam"))
      goto next;
		
    name = osync_xml_find_node(cur, "ParamName");
    content = osync_xml_find_node(cur, "Content");
    if (!xmlStrcmp(name, BAD_CAST paramname)) {
      osync_xml_node_add(node, newname, (char *) content);
      osync_xml_node_remove_unknown_mark(node);
			
      xmlUnlinkNode(cur);
      xmlFreeNode(cur);
      xmlFree(name);
      xmlFree(content);
      return;
    }
		
    xmlFree(name);
    xmlFree(content);
			
  next:;
    cur = cur->next;
  }
}

osync_bool osync_xml_has_property_full(xmlNode *parent, const char *name, const char *data)
{
  if (osync_xml_has_property(parent, name))
    return (strcmp((char*)xmlGetProp(parent, (xmlChar*)name), data) == 0);
  return FALSE;
}

char *osync_xml_find_property(xmlNode *parent, const char *name)
{
  return (char*)xmlGetProp(parent, (xmlChar*)name);
}

osync_bool osync_xml_has_property(xmlNode *parent, const char *name)
{
  return (xmlHasProp(parent, (xmlChar*)name) != NULL);
}

static osync_bool osync_xml_compare_node(xmlNode *leftnode, xmlNode *rightnode)
{
  xmlNode *rightstartnode = NULL;
  osync_trace(TRACE_ENTRY, "%s(%p:%s, %p:%s)", __func__, leftnode, leftnode->name, rightnode, rightnode->name);
  if (xmlStrcmp(leftnode->name, rightnode->name)) {
    osync_trace(TRACE_EXIT, "%s: FALSE: Different Name", __func__);
    return FALSE;
  }
	
  leftnode = leftnode->children;
  rightnode = rightnode->children;
  rightstartnode = rightnode;
	
  if (!leftnode && !rightnode) {
    osync_trace(TRACE_EXIT, "%s: TRUE. Both 0", __func__);
    return TRUE;
  }
	
  if (!leftnode || !rightnode) {
    osync_trace(TRACE_EXIT, "%s: FALSE. One 0", __func__);
    return FALSE;
  }
	
  do {
    xmlChar *leftcontent = NULL;
    if (!strcmp("UnknownParam", (char*)leftnode->name))
      continue;
    if (!strcmp("Order", (char*)leftnode->name))
      continue;
    rightnode = rightstartnode;
    leftcontent = xmlNodeGetContent(leftnode);
		
    do {
      xmlChar *rightcontent = NULL;
      if (!strcmp("UnknownParam", (char*)rightnode->name))
        continue;
      rightcontent = xmlNodeGetContent(rightnode);
			
      osync_trace(TRACE_INTERNAL, "leftcontent %s (%s), rightcontent %s (%s)", leftcontent, leftnode->name, rightcontent, rightnode->name);
      if (leftcontent == rightcontent) {
        xmlFree(rightcontent);
        goto next;
      }
      if (!leftcontent || !rightcontent) {
        osync_trace(TRACE_EXIT, "%s: One is empty", __func__);
        return FALSE;
      }
      if (!xmlStrcmp(leftcontent, rightcontent)) {
        xmlFree(rightcontent);
        goto next;
      }
      xmlFree(rightcontent);
    } while ((rightnode = rightnode->next));
    osync_trace(TRACE_EXIT, "%s: Could not match one", __func__);
    xmlFree(leftcontent);
    return FALSE;
  next:;
    xmlFree(leftcontent);
  } while ((leftnode = leftnode->next));
	
  osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
  return TRUE;
}

OSyncConvCmpResult osync_xml_compare(xmlDoc *leftinpdoc, xmlDoc *rightinpdoc, OSyncXMLScore *scores, int default_score, int treshold)
{
  int z = 0, i = 0, n = 0;
  int res_score = 0;
  xmlDoc *leftdoc = NULL;
  xmlDoc *rightdoc = NULL;

  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, leftinpdoc, rightinpdoc, scores);
	
  leftdoc = xmlCopyDoc(leftinpdoc, TRUE);
  rightdoc = xmlCopyDoc(rightinpdoc, TRUE);
	
  osync_trace(TRACE_INTERNAL, "Comparing given score list");
  while (scores && scores[z].path) {
    OSyncXMLScore *score = &scores[z];
    xmlXPathObject *leftxobj = osync_xml_get_nodeset(leftdoc, score->path);
    xmlXPathObject *rightxobj = osync_xml_get_nodeset(rightdoc, score->path);
		
    xmlNodeSet *lnodes = leftxobj->nodesetval;
    xmlNodeSet *rnodes = rightxobj->nodesetval;
		
    int lsize = (lnodes) ? lnodes->nodeNr : 0;
    int rsize = (rnodes) ? rnodes->nodeNr : 0;
    z++;
    osync_trace(TRACE_INTERNAL, "parsing next path %s", score->path);
		
    if (!score->value) {
      for (i = 0; i < lsize; i++) {
        xmlUnlinkNode(lnodes->nodeTab[i]);
        xmlFreeNode(lnodes->nodeTab[i]);
        lnodes->nodeTab[i] = NULL;
      }
			
      for (n = 0; n < rsize; n++) {
        xmlUnlinkNode(rnodes->nodeTab[n]);
        xmlFreeNode(rnodes->nodeTab[n]);
        rnodes->nodeTab[n] = NULL;
      }
    } else {
      for (i = 0; i < lsize; i++) {
        if (!lnodes->nodeTab[i])
          continue;
        for (n = 0; n < rsize; n++) {
          xmlChar *lcontent = NULL;
          xmlChar *rcontent = NULL;
          if (!rnodes->nodeTab[n])
            continue;
          lcontent = osync_xml_find_node(lnodes->nodeTab[i], "Content");
          rcontent = osync_xml_find_node(rnodes->nodeTab[n], "Content"); 
          osync_trace(TRACE_INTERNAL, "cmp %i:%s (%s), %i:%s (%s)", i, lnodes->nodeTab[i]->name, lcontent, n, rnodes->nodeTab[n]->name, rcontent);
          xmlFree(lcontent);
          xmlFree(rcontent);

          if (osync_xml_compare_node(lnodes->nodeTab[i], rnodes->nodeTab[n])) {
            osync_trace(TRACE_INTERNAL, "Adding %i for %s", score->value, score->path);
            res_score += score->value;
            xmlUnlinkNode(lnodes->nodeTab[i]);
            xmlFreeNode(lnodes->nodeTab[i]);
            lnodes->nodeTab[i] = NULL;
            xmlUnlinkNode(rnodes->nodeTab[n]);
            xmlFreeNode(rnodes->nodeTab[n]);
            rnodes->nodeTab[n] = NULL;
            goto next;
          }
        }
        osync_trace(TRACE_INTERNAL, "Subtracting %i for %s", score->value, score->path);
        res_score -= score->value;
      next:;
      }
      for(i = 0; i < rsize; i++) {
        if (!rnodes->nodeTab[i])
          continue;
        res_score -= score->value;
      }
    }
		
    xmlXPathFreeObject(leftxobj);
    xmlXPathFreeObject(rightxobj);
  }

  { /* Block for new variables */
    xmlXPathObject *leftxobj = osync_xml_get_nodeset(leftdoc, "/*/*");
    xmlXPathObject *rightxobj = osync_xml_get_nodeset(rightdoc, "/*/*");
	
    xmlNodeSet *lnodes = leftxobj->nodesetval;
    xmlNodeSet *rnodes = rightxobj->nodesetval;
	
    int lsize = (lnodes) ? lnodes->nodeNr : 0;
    int rsize = (rnodes) ? rnodes->nodeNr : 0;
    osync_bool same = TRUE;
    osync_trace(TRACE_INTERNAL, "Comparing remaining list");

    for(i = 0; i < lsize; i++) {		
      for (n = 0; n < rsize; n++) {
        xmlChar *lcontent = NULL;
        xmlChar *rcontent = NULL;

        if (!rnodes->nodeTab[n])
          continue;

        lcontent = osync_xml_find_node(lnodes->nodeTab[i], "Content");
        rcontent = osync_xml_find_node(rnodes->nodeTab[n], "Content"); 

        osync_trace(TRACE_INTERNAL, "cmp %i:%s (%s), %i:%s (%s)", i, lnodes->nodeTab[i]->name, lcontent, n, rnodes->nodeTab[n]->name, rcontent);

        xmlFree(lcontent);
        xmlFree(rcontent);

        if (osync_xml_compare_node(lnodes->nodeTab[i], rnodes->nodeTab[n])) {
          xmlUnlinkNode(lnodes->nodeTab[i]);
          xmlFreeNode(lnodes->nodeTab[i]);
          lnodes->nodeTab[i] = NULL;
          xmlUnlinkNode(rnodes->nodeTab[n]);
          xmlFreeNode(rnodes->nodeTab[n]);
          rnodes->nodeTab[n] = NULL;
          osync_trace(TRACE_INTERNAL, "Adding %i", default_score);
          res_score += default_score;
          goto next2;
        }
      }
      osync_trace(TRACE_INTERNAL, "Subtracting %i", default_score);
      res_score -= default_score;
      same = FALSE;
      //goto out;
    next2:;
    }
	
    for(i = 0; i < lsize; i++) {
      if (!lnodes->nodeTab[i])
        continue;
      osync_trace(TRACE_INTERNAL, "left remaining: %s", lnodes->nodeTab[i]->name);
      same = FALSE;
      goto out;
    }
	
    for(i = 0; i < rsize; i++) {
      if (!rnodes->nodeTab[i])
        continue;
      osync_trace(TRACE_INTERNAL, "right remaining: %s", rnodes->nodeTab[i]->name);
      same = FALSE;
      goto out;
    }
  out:
    xmlXPathFreeObject(leftxobj);
    xmlXPathFreeObject(rightxobj);

    osync_xml_free_doc(leftdoc);
    osync_xml_free_doc(rightdoc);	
        

    osync_trace(TRACE_INTERNAL, "Result is: %i, Treshold is: %i", res_score, treshold);
    if (same) {
      osync_trace(TRACE_EXIT, "%s: SAME", __func__);
      return OSYNC_CONV_DATA_SAME;
    }
  } /* Block for new variables */
  if (res_score >= treshold) {
    osync_trace(TRACE_EXIT, "%s: SIMILAR", __func__);
    return OSYNC_CONV_DATA_SIMILAR;
  }
  osync_trace(TRACE_EXIT, "%s: MISMATCH", __func__);
  return OSYNC_CONV_DATA_MISMATCH;
}

/**
 * @brief Dumps the XML tree to a string 
 * @param doc the XML doc value 
 * @return String of XML the tree (the caller is responsible for freeing) 
 */
char *osync_xml_write_to_string(xmlDoc *doc)
{
  xmlChar *temp = NULL;
  int size = 0;
  xmlKeepBlanksDefault(0);
  xmlDocDumpFormatMemoryEnc(doc, &temp, &size, NULL, 1);
  return (char *)temp;
}

osync_bool osync_xml_copy(const char *input, unsigned int inpsize, char **output, unsigned int *outpsize, OSyncError **error)
{
  xmlDoc *doc = (xmlDoc *)(input);
  xmlDoc *newdoc = xmlCopyDoc(doc, TRUE);
  *output = (char *)newdoc;
  *outpsize = sizeof(newdoc);
  return TRUE;
}

osync_bool osync_xml_marshal(const char *input, unsigned int inpsize, OSyncMessage *message, OSyncError **error)
{
  xmlDoc *doc = (xmlDoc*)input;
  xmlChar *result;
  int size;
  xmlDocDumpMemory(doc, &result, &size);
  osync_message_write_buffer(message, result, size);
	
  return TRUE;
}

osync_bool osync_xml_demarshal(OSyncMessage *message, char **output, unsigned int *outpsize, OSyncError **error)
{
  void *input = NULL;
  int size = 0;
  xmlDoc *doc = NULL;
  osync_message_read_buffer(message, &input, &size);
	
  doc = xmlParseMemory((char *)input, size);
  if (!doc) {
    osync_error_set(error, OSYNC_ERROR_GENERIC, "Invalid XML data received");
    goto error;
  }

  *output = (char*)doc;
  *outpsize = sizeof(*doc);
  return TRUE;

 error:
  return FALSE;
}

osync_bool osync_xml_validate_document(xmlDocPtr doc, char *schemafilepath)
{
  int rc = 0;
  xmlSchemaParserCtxtPtr xmlSchemaParserCtxt = NULL;
  xmlSchemaPtr xmlSchema = NULL;
  xmlSchemaValidCtxtPtr xmlSchemaValidCtxt = NULL;

  osync_assert(doc);
  osync_assert(schemafilepath);
	
  xmlSchemaParserCtxt = xmlSchemaNewParserCtxt(schemafilepath);
  xmlSchema = xmlSchemaParse(xmlSchemaParserCtxt);
  xmlSchemaFreeParserCtxt(xmlSchemaParserCtxt);

  xmlSchemaValidCtxt = xmlSchemaNewValidCtxt(xmlSchema);
  if (xmlSchemaValidCtxt == NULL) {
    xmlSchemaFree(xmlSchema);
    rc = 1;
  }else{
    /* Validate the document */
    rc = xmlSchemaValidateDoc(xmlSchemaValidCtxt, doc);
    xmlSchemaFree(xmlSchema);
    xmlSchemaFreeValidCtxt(xmlSchemaValidCtxt);
  }

  if(rc != 0)
    return FALSE;
  return TRUE;
}

/**
 * @brief Help method which return the content of a xmlNode
 * @param node The pointer to a xmlNode
 * @return The value of the xmlNode or a empty string
 */
xmlChar *osync_xml_node_get_content(xmlNodePtr node)
{
  if(node->children && node->children->content)
    return node->children->content;
		
  return (xmlChar *)"";
}

/**
 * @brief Help method which return the content of a xmlAttr
 * @param node The pointer to a xmlAttr
 * @return The value of the xmlAttr or a empty string
 */
xmlChar *osync_xml_attr_get_content(xmlAttrPtr node)
{
  if(node->children && node->children->content)
    return node->children->content;
		
  return (xmlChar *)"";
}

/*! @brief Opens a xml document
 * 
 * Opens a xml document
 * 
 * @param doc Pointer to a xmldoc
 * @param cur The pointer to the first node
 * @param path The path of the document
 * @param topentry the name of the top node
 * @param error Pointer to a error struct
 * @returns TRUE if successful, FALSE otherwise
 * 
 */
osync_bool osync_xml_open_file(xmlDocPtr *doc, xmlNodePtr *cur, const char *path, const char *topentry, OSyncError **error)
{
  if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
    osync_error_set(error, OSYNC_ERROR_IO_ERROR, "File %s does not exist", path);
    return FALSE;
  }
	
  *doc = xmlParseFile(path);
  if (!*doc) {
    osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Could not open: %s", path);
    goto error;
  }

  *cur = xmlDocGetRootElement(*doc);
  if (!*cur) {
    osync_error_set(error, OSYNC_ERROR_IO_ERROR, "%s seems to be empty", path);
    goto error_free_doc;
  }

  if (xmlStrcmp((*cur)->name, (const xmlChar *) topentry)) {
    osync_error_set(error, OSYNC_ERROR_IO_ERROR, "%s seems not to be a valid configfile.\n", path);
    goto error_free_doc;
  }

  *cur = (*cur)->xmlChildrenNode;
  return TRUE;

 error_free_doc:
  osync_xml_free_doc(*doc);
 error:
  osync_trace(TRACE_ERROR, "%s: %s", __func__, osync_error_print(error));
  return FALSE;
}

