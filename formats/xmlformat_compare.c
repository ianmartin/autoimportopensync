/*
 * xmlformat_compare - comparsion logic of xmlformat
 * Copyright (C) 2006  Daniel Friedrich <daniel.friedrich@opensync.org>
 * Copyright (C) 2007  Daniel Gollub <dgollub@suse.de>
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

#include "xmlformat.h"

#include <libxml/xmlstring.h>
#include <libxml/tree.h>

#include "opensync/xmlformat/opensync_xmlformat_internals.h"


#include "opensync/xmlformat/opensync_xmlfield_private.h"	/* FIXME: direct include of private header */


/**
 * @brief Search for the points in the sorted OSyncXMLPoints array for a given fieldname.
 * @param points The sorted points array
 * @param cur_pos Pointer to the actual line in the points array
 * @param basic_points Points which should be returned if fieldname will not found in the points array 
 * @param fieldname The name of the field for which the points should be returned
 * @returns The points for the fieldname
 */
int xmlformat_get_points(OSyncXMLPoints *points, int* cur_pos, int basic_points, const char* fieldname)
{
  for(; points[(*cur_pos)].fieldname; (*cur_pos)++) {
    int res = strcmp(points[(*cur_pos)].fieldname, fieldname);
    if(res == 0) 
      return points[(*cur_pos)].points;
    if(res > 0)
      return basic_points;
  }

  return basic_points;
}

/**
 * @brief Search for the points in the sorted OSyncXMLPoints array for the fieldname of xmlfield and handle ignored fields.
 * 
 *  This uses xmlformat_get_points() but handle the case if the field needs to be ignored.
 *  Ignored fields doesn't have influence on the compare result. This is needed to keep the compare 
 *  result SAME if an ignored fields are the only differences between the entries.
 *
 *  0 Points got returned for ignored fields to avoid influence of the collected_points value and let
 *  collected_points not raise over the threshold value. This could change the compare result to SIMILAR,
 *  even if the entry should  MISMATCH, this should be avoided. This happens when the number of ignored fiels
 *  is the greather equal as the thresold value.
 *
 * @param xmlfield Pointer to xmlfield
 * @param points The sorted points array
 * @param cur_pos Pointer to the actual line in the points array. Gets incremented if the xmlfield requires this! 
 * @param basic_points Points which should be returned if fieldname will not found in the points array 
 * @param same Pointer ot the compare result flag SAME, which got not set to FALSE if the fields should be ignored.
 * @returns The points for the fieldname. If the field should be ignored 0 points got retunred.
 */
static int xmlformat_subtract_points(OSyncXMLField *xmlfield, OSyncXMLPoints *points, int *cur_pos, int basic_points, int *same) {
  int p = 0;
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %i, %p)", __func__, xmlfield, points, cur_pos, basic_points, same);
  p = xmlformat_get_points(points, cur_pos, basic_points, osync_xmlfield_get_name(xmlfield));

  /* Stay with SAME as compare result and don't substract any points - if fields should be ignored */
  if (p != -1) {
    osync_trace(TRACE_INTERNAL, "Not same anymore - \"%s\" field differs!", osync_xmlfield_get_name(xmlfield));
    *same = FALSE;
  } else {
    osync_trace(TRACE_INTERNAL, "Ignored field: %s", osync_xmlfield_get_name(xmlfield));
    p = 0;
  }

  osync_trace(TRACE_EXIT, "%s: %i", __func__, p);
  return p;
}

/**
 * @brief Help method which return the content of a xmlNode
 * @param node The pointer to a xmlNode
 * @return The value of the xmlNode or a empty string
 */
static xmlChar *xml_node_get_content(xmlNodePtr node)
{
  if (node->children && node->children->content)
    return node->children->content;
		
  return (xmlChar *)"";
}

/**
 * @brief Compares two xmlfield objects with each other
 * @param xmlfield1 The pointer to a xmlformat object
 * @param xmlfield2 The pointer to a xmlformat object
 * @return TRUE if both xmlfield objects are the same otherwise FALSE
 */
osync_bool xmlfield_compare(OSyncXMLField *xmlfield1, OSyncXMLField *xmlfield2)
{
  int i;	
  osync_bool same;
  xmlNodePtr key1 = NULL;
  xmlNodePtr key2 = NULL;


  osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, xmlfield1, xmlfield2);
  osync_assert(xmlfield1);
  osync_assert(xmlfield2);
	
  if(strcmp(osync_xmlfield_get_name(xmlfield1), osync_xmlfield_get_name(xmlfield2))) {
    osync_trace(TRACE_EXIT, "%s: %i", __func__, FALSE);
    return FALSE;
  }

  same = TRUE;
  key1 = xmlfield1->node->children;
  key2 = xmlfield2->node->children;
	
  while(same)
    {
      GSList *keylist1 = NULL;
      GSList *keylist2 = NULL;
      const char *curkeyname = NULL;

      if(key1 == NULL && key2 == NULL) {
        break;
      }
			
      if(key1 == NULL || key2 == NULL) {
        same = FALSE;
        break;	
      }
		
      curkeyname = (const char *)key1->name;
      do {
        keylist1 = g_slist_prepend(keylist1, key1);
        key1 = key1->next;
        if(key1 == NULL)
          break;
        i = strcmp((const char *)key1->name, curkeyname);
      } while(i == 0);
		
      do {
        keylist2 = g_slist_prepend(keylist2, key2);
        key2 = key2->next;
        if(key2 == NULL)
          break;
        i = strcmp((const char *)key2->name, curkeyname);
      } while(i == 0);	

      do{
        GSList *cur_list1 = NULL;
        GSList *cur_list2 = NULL;

        /* both lists must have the same length */
        if(g_slist_length(keylist1) != g_slist_length(keylist2)) {
          osync_trace(TRACE_INTERNAL, "It's not the same anymore...");
          same = FALSE;
          break;
        }

        do {
          cur_list1 = keylist1;
          cur_list2 = keylist2;

          do {
            if(!xmlStrcmp(xml_node_get_content(cur_list1->data), xml_node_get_content(cur_list2->data)))
              break;
            cur_list2 = g_slist_next(cur_list2);
            if(cur_list2 == NULL) {
              same = FALSE;
              break;
            }
          }while(1);

          if(same) {
            keylist1 = g_slist_delete_link(keylist1, cur_list1);
            keylist2 = g_slist_delete_link(keylist2, cur_list2);
          }else
            break;

        }while(keylist1 != NULL);
      }while(0);

      if(keylist1)
        g_slist_free(keylist1);
      if(keylist2)
        g_slist_free(keylist2);
    }

  /* now we check if the attributes are equal */
  do {
    int i1 = 0, i2 = 0;
    if(!same)
      break;

    i1 =osync_xmlfield_get_attr_count(xmlfield1);
    i2 =osync_xmlfield_get_attr_count(xmlfield2);

    if(i1 != i2) {
      same = FALSE;
      break;
    }

    for(i=0; i < i1; i++) {
      const char *attrvalue = osync_xmlfield_get_attr(xmlfield2, osync_xmlfield_get_nth_attr_name(xmlfield1, i));
      if( attrvalue == NULL ||
          strcmp(attrvalue, osync_xmlfield_get_nth_attr_value(xmlfield1, i))) {
        same = FALSE;
        break;
      }
    }
  } while(0);
  osync_trace(TRACE_EXIT, "%s: %i", __func__, same);
  return same;
}

/**
 * @brief Compares two xmlfield objects with each other of similarity
 * @param xmlfield1 The pointer to a xmlformat object
 * @param xmlfield2 The pointer to a xmlformat object
 * @param keys OSyncXMLPoints::keys
 * @return TRUE if both xmlfield objects are the similar otherwise FALSE
 */
osync_bool xmlfield_compare_similar(OSyncXMLField *xmlfield1, OSyncXMLField *xmlfield2, char* keys[])
{
  osync_bool res = TRUE;
  xmlNodePtr node1, node2;
  int i;
  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, xmlfield1, xmlfield2, keys);
  osync_assert(xmlfield1);
  osync_assert(xmlfield2);

  if(strcmp((const char *) osync_xmlfield_get_name(xmlfield1), (const char *) osync_xmlfield_get_name(xmlfield2)) != 0)
    res = FALSE;

  node1 = xmlfield1->node->children;
  node2 = xmlfield2->node->children;

  for(i=0; keys[i]; i++) {
    GSList *list1;
    GSList *list2;
    GSList *cur_list1;
    GSList *cur_list2;

    list1 = NULL;
    list2 = NULL;

    while(node1 != NULL) {
      if(strcmp(keys[i], (const char *)node1->name) == 0)
        list1 = g_slist_prepend(list1, node1);
      node1 = node1->next;
    };

    while(node2 != NULL) {
      if(strcmp(keys[i], (const char *)node2->name) == 0)
        list2 = g_slist_prepend(list2, node2);
      node2 = node2->next;
    };

    while(list1 != NULL)
      {
        cur_list1 = list1;
        cur_list2 = list2;

        if(cur_list2 == NULL) {
          res = FALSE;
          break;
        }

        while(xmlStrcmp(xml_node_get_content((xmlNodePtr)cur_list1->data),
                        xml_node_get_content((xmlNodePtr)cur_list2->data))) {
          cur_list2 = g_slist_next(cur_list2);
          if(cur_list2 == NULL) {
            res = FALSE;
            break;
          }
        }

        if(res) {
          list1 = g_slist_delete_link(list1, cur_list1);
          list2 = g_slist_delete_link(list2, cur_list2);
        }else
          break;
      }
    if(list2 != NULL)
      res = FALSE;

    if(!res) {
      if(list1 != NULL)
        g_slist_free(list1);
      if(list2 != NULL)
        g_slist_free(list2);
    }
  }
  osync_trace(TRACE_EXIT, "%s: %i", __func__, res);
  return res;
}


/**
 * @brief Compares two xmlformat objects with each other
 * @param xmlformat1 The pointer to a xmlformat object
 * @param xmlformat2 The pointer to a xmlformat object
 * @param points The sorted points array
 * @param basic_points Points which should be used if a xmlfield name is not found in the points array
 * @param threshold If the two xmlformats are not the same, then this value will decide if the two xmlformats are similar 
 * @return One of the values of the OSyncConvCmpResult enumeration
 */
OSyncConvCmpResult xmlformat_compare(OSyncXMLFormat *xmlformat1, OSyncXMLFormat *xmlformat2, OSyncXMLPoints points[], int basic_points, int threshold)
{
  int res, collected_points, cur_pos;
  OSyncXMLField *xmlfield1 = NULL;
  OSyncXMLField *xmlfield2 = NULL;
  osync_bool same = TRUE;

  osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %i, %i)", __func__, xmlformat1, xmlformat2, points, basic_points, threshold);

  xmlfield1 = osync_xmlformat_get_first_field(xmlformat1);
  xmlfield2 = osync_xmlformat_get_first_field(xmlformat2);

  cur_pos = 0;
  collected_points = 0;

  while(xmlfield1 != NULL || xmlfield2 != NULL)
    {

      /* subtract points for xmlfield2*/
      if(xmlfield1 == NULL) {
        collected_points -= xmlformat_subtract_points(xmlfield2, points, &cur_pos, basic_points, &same);
        xmlfield2 = osync_xmlfield_get_next(xmlfield2);
        continue;
      }

      /* subtract points for xmlfield1*/
      if(xmlfield2 == NULL) {
        collected_points -= xmlformat_subtract_points(xmlfield1, points, &cur_pos, basic_points, &same);
        xmlfield1 = osync_xmlfield_get_next(xmlfield1);
        continue;
      }

      res = strcmp(osync_xmlfield_get_name(xmlfield1), osync_xmlfield_get_name(xmlfield2));
      osync_trace(TRACE_INTERNAL, "result of strcmp(): %i (%s || %s)", res, osync_xmlfield_get_name(xmlfield1), osync_xmlfield_get_name(xmlfield2));

      /* subtract points for xmlfield1*/
      if(res < 0) {
        collected_points -= xmlformat_subtract_points(xmlfield1, points, &cur_pos, basic_points, &same);
        xmlfield1 = osync_xmlfield_get_next(xmlfield1);
        continue;
      }

      /* subtract points for xmlfield2*/
      if(res > 0) {
        collected_points -= xmlformat_subtract_points(xmlfield2, points, &cur_pos, basic_points, &same);
        xmlfield2 = osync_xmlfield_get_next(xmlfield2);
        continue;
      }

      /* make lists and compare */
      if(res == 0)
        {
          GSList *fieldlist1 = NULL;
          GSList *fieldlist2 = NULL;

          const char *curfieldname = osync_xmlfield_get_name(xmlfield1);

          /* get the points*/
          int p = xmlformat_get_points(points, &cur_pos, basic_points, curfieldname);

          /* don't compare both fields if they should be ignore to avoid influence of the compare result */
          if (p == -1) {
            xmlfield1 = osync_xmlfield_get_next(xmlfield1);
            xmlfield2 = osync_xmlfield_get_next(xmlfield2);
            continue;
          }

          do {
            fieldlist1 = g_slist_prepend(fieldlist1, xmlfield1);
            xmlfield1 = osync_xmlfield_get_next(xmlfield1);
            if(xmlfield1 == NULL)
              break;
            res = strcmp(osync_xmlfield_get_name(xmlfield1), curfieldname);
          } while(res == 0);

          do {
            fieldlist2 = g_slist_prepend(fieldlist2, xmlfield2);
            xmlfield2 = osync_xmlfield_get_next(xmlfield2);
            if(xmlfield2 == NULL)
              break;
            res = strcmp(osync_xmlfield_get_name(xmlfield2), curfieldname);
          } while(res == 0);

          do {

            /* if same then compare and give points*/
            do {
              GSList *cur_list1;
              GSList *cur_list2;

              if(!same)
                break;

              /* both lists must have the same length */
              if(g_slist_length(fieldlist1) != g_slist_length(fieldlist2)) {
                same = FALSE;
                osync_trace(TRACE_INTERNAL, "both list don't have the same length");
                break;
              }

              do {
                cur_list1 = fieldlist1;
                cur_list2 = fieldlist2;

                do {
                  if(xmlfield_compare((OSyncXMLField *)cur_list1->data, (OSyncXMLField *)cur_list2->data) == TRUE)
                    break;
                  cur_list2 = g_slist_next(cur_list2);
                  if(cur_list2 == NULL) {
                    same = FALSE;
                    osync_trace(TRACE_INTERNAL, "one field is alone: %s", osync_xmlfield_get_name(cur_list1->data));
                    break;
                  }
                }while(1);

                if(same) {
                  /* add the points */
                  osync_trace(TRACE_INTERNAL, "add %i point(s) for same fields: %s", p, curfieldname);
                  collected_points += p;
                  fieldlist1 = g_slist_delete_link(fieldlist1, cur_list1);
                  fieldlist2 = g_slist_delete_link(fieldlist2, cur_list2);
                }else
                  break;

              }while(fieldlist1 != NULL);
            } while(0);

            /* if similar then compare and give points*/
            do {
              GSList *cur_list1;
              GSList *cur_list2;
              osync_bool found;
              int subtracted_count;

              if(same)
                break;
              /* if no points to add or to subtract we need no compair of similarity */
              if(!p) {
                g_slist_free(fieldlist1);
                g_slist_free(fieldlist2);
                fieldlist1 = NULL;
                fieldlist2 = NULL;
                break;
              }

              subtracted_count = 0;
              do {
                found = FALSE;
                cur_list1 = fieldlist1;
                cur_list2 = fieldlist2;

                while(cur_list2) {

                  if(xmlfield_compare_similar((OSyncXMLField *)cur_list1->data,
                                                (OSyncXMLField *)cur_list2->data,
                                                points[cur_pos].keys) == TRUE) {
                    found = TRUE;
                    break;
                  }

                  cur_list2 = g_slist_next(cur_list2);
                }

                /* add or subtract the points */
                if(found) {
                  /* add the points */
                  osync_trace(TRACE_INTERNAL, "add %i point(s) for similiar field: %s", p, curfieldname);
                  collected_points += p;
                  fieldlist1 = g_slist_delete_link(fieldlist1, cur_list1);
                  fieldlist2 = g_slist_delete_link(fieldlist2, cur_list2);
                }else{
                  /* subtract the points */
                  osync_trace(TRACE_INTERNAL, "subtracting %i point(s) for missing field: %s", p, curfieldname);
                  collected_points -= p;
                  subtracted_count++;
                  fieldlist1 = g_slist_delete_link(fieldlist1, cur_list1);
                }
              }while(fieldlist1 != NULL);

              /* subtract the points for the remaining elements in the list2 */
              while(fieldlist2) {
                /* subtract the points */
                if(subtracted_count > 0) {
                  subtracted_count--;
                } else {
                  osync_trace(TRACE_INTERNAL, "subtracting %i point(s) for remaining field: %s", p, curfieldname);
                  collected_points -= p;
                }
                fieldlist2 = g_slist_delete_link(fieldlist2, fieldlist2);
              }

            }while(0);

          }while(0);

          /* the lists should not exist */
          g_assert(!fieldlist1);
          g_assert(!fieldlist2);
        }
    };

  osync_trace(TRACE_INTERNAL, "Result is: %i, Treshold is: %i", collected_points, threshold);
  if (same) {
    osync_trace(TRACE_EXIT, "%s: SAME", __func__);
    return OSYNC_CONV_DATA_SAME;
  }
  if (collected_points >= threshold) {
    osync_trace(TRACE_EXIT, "%s: SIMILAR", __func__);
    return OSYNC_CONV_DATA_SIMILAR;
  }
  osync_trace(TRACE_EXIT, "%s: MISMATCH", __func__);
  return OSYNC_CONV_DATA_MISMATCH;
}

