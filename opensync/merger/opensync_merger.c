/*
 * libopensync - A synchronization framework
 * Copyright (C) 2006  NetNix Finland Ltd <netnix@netnix.fi>
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
 * Author: Daniel Friedrich <daniel.friedrich@opensync.org>
 * 
 */
 
#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-merger.h"
#include "opensync-merger_internals.h"

/**
 * @defgroup OSyncMergerPrivateAPI OpenSync Merger Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncMerger
 * 
 */
/*@{*/

/*@}*/

/**
 * @defgroup OSyncMergerAPI OpenSync Merger
 * @ingroup OSyncPublic
 * @brief The public part of the OSyncMerger
 * 
 */
/*@{*/

/**
 * @brief Creates a new merger object
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated merger object or NULL in case of error
 */
OSyncMerger *osync_merger_new(OSyncCapabilities *capabilities, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, capabilities, error);
	osync_assert(capabilities);
	
	OSyncMerger *merger = osync_try_malloc0(sizeof(OSyncMerger), error);
	if(!merger) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	
	merger->ref_count = 1;
	osync_capabilities_ref(capabilities);
	merger->capabilities = capabilities;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, merger);
	return merger;
}

/**
 * @brief Increments the reference counter
 * @param merger The pointer to a merger object
 */
void osync_merger_ref(OSyncMerger *merger)
{
	osync_assert(merger);
	
	g_atomic_int_inc(&(merger->ref_count));
}

/**
 * @brief Decrement the reference counter. The merger object will 
 *  be freed if there is no more reference to it.
 * @param merger The pointer to a merger object
 */
void osync_merger_unref(OSyncMerger *merger)
{
	osync_assert(merger);
			
	if (g_atomic_int_dec_and_test(&(merger->ref_count))) {
		osync_capabilities_unref(merger->capabilities);
		g_free(merger);
	}
}

/**
 * @brief Merge all xmlfields from the entire xmlformat into the
 *  xmlformat if they are not listed in the capabilities.
 * @param merger The pointer to a merger object
 * @param xmlformat The pointer to a xmlformat object
 * @param entire The pointer to a entire xmlformat object
 */
void osync_merger_merge(OSyncMerger *merger, OSyncXMLFormat *xmlformat, OSyncXMLFormat *entire)
{
	OSyncXMLField *old_cur, *new_cur, *tmp;
	OSyncCapability *cap_cur;
	int ret;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, merger, xmlformat, entire);
	osync_assert(merger);
	osync_assert(xmlformat);
	osync_assert(entire);
	
	 cap_cur = osync_capabilities_get_first(merger->capabilities, osync_xmlformat_get_objtype(xmlformat));
	 if(!cap_cur)
	 	return;
	 	
	 new_cur = osync_xmlformat_get_first_field(xmlformat);
	 old_cur = osync_xmlformat_get_first_field(entire);
	 while(old_cur != NULL)
	 {		 	
	 	ret = strcmp(osync_xmlfield_get_name(new_cur), osync_xmlfield_get_name(old_cur));
	 	if(ret < 0) {
	 		if(new_cur->next != NULL) {
				new_cur = osync_xmlfield_get_next(new_cur);
				continue;
	 		}
		 }
		 
	 	if(cap_cur == NULL)	{
			tmp = old_cur;
			old_cur = osync_xmlfield_get_next(old_cur);
			if(ret >= 0) {
				osync_xmlfield_adopt_xmlfield_before_field(new_cur, tmp);
			} else {
				osync_xmlfield_adopt_xmlfield_after_field(new_cur, tmp);
			}
			continue;		 		
	 	}

 		ret = strcmp(osync_capability_get_name(cap_cur), osync_xmlfield_get_name(old_cur));
	 	if(ret == 0)
	 	{
	 		/* 
	 		 * now we have to merge the key/value pairs (second level)
	 		 * we see the second level as sorted and with the same fields (exception the last key)
	 		 * KEY(new)		Capabilities		KEY(old)
	 		 * KEY1				KEY1				KEY1
	 		 * KEY2(empty)		KEY3				KEY2
	 		 * KEY2(empty)							KEY2
	 		 * KEY3									KEY3
	 		 * 										KEY4
	 		 * 										KEY4
	 		 */
	 		if( osync_capability_has_key(cap_cur) &&
	 			!strcmp(osync_capability_get_name(cap_cur), osync_xmlfield_get_name(new_cur))) 
	 		{
				xmlNodePtr cap_tmp, new_tmp;
				xmlNodePtr cap_node = cap_cur->node->children;
				xmlNodePtr new_node = new_cur->node->children;
				xmlNodePtr old_node = old_cur->node->children;
				xmlNodePtr new_par_node = new_cur->node;
			
				while(old_node) {
					GSList *list, *tmp;
					int i, size;
					const xmlChar *curkeyname;
					
					size=0;
					curkeyname = old_node->name;
					list = NULL;
					do {
						list = g_slist_prepend(list, old_node);
						size++;
						old_node = old_node->next;
						if(old_node == NULL)
							break;
						i = xmlStrcmp(old_node->name, curkeyname);
					} while(i == 0);

					/* search for the curkeyname in the capabilities */
					for(cap_tmp = cap_node; cap_tmp != NULL; cap_tmp = cap_tmp->next) {
						if(!xmlStrcmp(cap_tmp->name, curkeyname)) {
							cap_node = cap_tmp;
							break;
						}
					}

					if(cap_tmp) { 
						/* curkeyname was found in the capibilities */
						/* we have to set the new_node ptr to the right position */
						for(; new_node && size > 0; size--) {
							new_node = new_node->next;
						}
					}else{
						/* curkeyname was _not_ found in the capabilities */
						/* link all key/value pairs with the key curkeyname to the the new_node */
						
						list = g_slist_reverse(list);

						if(new_node == NULL) {
							for(tmp=list; tmp != NULL; tmp = g_slist_next(tmp)) {
								xmlUnlinkNode(tmp->data);
								xmlDOMWrapAdoptNode(NULL, ((xmlNodePtr)tmp->data)->doc, tmp->data, new_node->doc, new_par_node, 0);
								//xmlAddChild(new_par_node, tmp->data);
							}
						}else{
							for(tmp=list; tmp != NULL; tmp = g_slist_next(tmp)) {
								xmlUnlinkNode(tmp->data);
								xmlDOMWrapAdoptNode(NULL, ((xmlNodePtr)tmp->data)->doc, tmp->data, new_node->doc, new_par_node, 0);
								xmlAddPrevSibling(new_node, tmp->data);
							}
							
							do{
								new_tmp = new_node;
								new_node = new_node->next;
								xmlUnlinkNode(new_tmp);
								xmlFreeNode(new_tmp);
								size--;
							}while(size > 0 && new_node);
							
						}
					}
					g_slist_free(list);
		 		}
	 		}
	 		old_cur = osync_xmlfield_get_next(old_cur);
			continue;		 		
	 	}
	 	else if(ret < 0)
	 	{
	 		cap_cur = osync_capability_get_next(cap_cur);
	 		continue;
	 	}
	 	else if(ret > 0)
	 	{
			tmp = old_cur;
			old_cur = osync_xmlfield_get_next(old_cur);
			osync_xmlfield_adopt_xmlfield_before_field(new_cur, tmp);
	 		continue;
	 	}
		g_assert_not_reached();
	 }
	 osync_trace(TRACE_EXIT, "%s: ", __func__);
}

/**
 * @brief Remove all xmlfields from the xmlformat if they are
 *  not listed in the capabilities.
 * @param merger The pointer to a merger object
 * @param xmlformat The pointer to a xmlformat object
 */
void osync_merger_demerge(OSyncMerger *merger, OSyncXMLFormat *xmlformat)
{
	OSyncXMLField *cur_xmlfield, *tmp;
	OSyncCapability *cur_capability;
	int rc;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, merger, xmlformat);
	osync_assert(merger);
	osync_assert(xmlformat);
	
	cur_capability = osync_capabilities_get_first(merger->capabilities, osync_xmlformat_get_objtype(xmlformat));
	cur_xmlfield = osync_xmlformat_get_first_field(xmlformat);
	
	if(cur_capability) /* if there is no capability - it means that the device can handle all xmlfields */
	{
		while(cur_xmlfield != NULL)
		{
	 		if(cur_capability == NULL) {
	 			/* delete all xmlfields */
	 			while(cur_xmlfield) {
	 				osync_trace(TRACE_INTERNAL, "Demerge XMLField: %s", osync_xmlfield_get_name(cur_xmlfield));
	 				tmp = osync_xmlfield_get_next(cur_xmlfield);
	 				osync_xmlfield_delete(cur_xmlfield);
	 				cur_xmlfield = tmp;
	 			}
	 			break; 			
	 		}
	 		
	 		rc = strcmp(osync_xmlfield_get_name(cur_xmlfield), osync_capability_get_name(cur_capability));
		 	if(rc == 0) {
		 		/* check the secound level here */
		 		if(osync_capability_has_key(cur_capability)) /* if there is no key - it means that the xmlfield can handle all keys */
		 		{
		 			int i, j=0;
		 			int capability_keys = osync_capability_get_key_count(cur_capability);
		 			int xmlfield_keys = osync_xmlfield_get_key_count(cur_xmlfield);
		 			
		 			for(i=0; i < xmlfield_keys; i++)
		 			{
		 				if(j == capability_keys) {
		 					for(; i < xmlfield_keys; i++) {
		 						osync_trace(TRACE_INTERNAL, "Demerge XMLField Key: %s->%s",	osync_xmlfield_get_name(cur_xmlfield), osync_xmlfield_get_nth_key_name(cur_xmlfield, i));
		 						osync_xmlfield_set_nth_key_value(cur_xmlfield, i, "");
		 					}
		 					break;
		 				}
		 				
		 				int krc = strcmp(osync_xmlfield_get_nth_key_name(cur_xmlfield, i), osync_capability_get_nth_key(cur_capability, j));
		 				if(krc == 0) {
		 					continue;
		 				}	
		 				if(krc > 0) {
		 					j++;
		 					continue;
		 				}
		 				if(krc < 0) {
		 					osync_trace(TRACE_INTERNAL, "Demerge XMLField Key: %s->%s",	osync_xmlfield_get_name(cur_xmlfield), osync_xmlfield_get_nth_key_name(cur_xmlfield, i));
		 					osync_xmlfield_set_nth_key_value(cur_xmlfield, i, "");
		 					continue;
		 				}
		 				g_assert_not_reached();
		 			}
		 		}
		 		cur_xmlfield = osync_xmlfield_get_next(cur_xmlfield);
		 		continue;
		 	}
		 	if(rc > 0) {
		 		cur_capability = osync_capability_get_next(cur_capability);
		 		continue;
		 	}
		 	if(rc < 0) {
				/* delete xmlfield */
				osync_trace(TRACE_INTERNAL, "Demerge XMLField: %s", osync_xmlfield_get_name(cur_xmlfield));
				tmp = osync_xmlfield_get_next(cur_xmlfield);
	 			osync_xmlfield_delete(cur_xmlfield);
	 			cur_xmlfield = tmp;
	 			continue;
		 	}
			g_assert_not_reached();
		}
	}
	osync_trace(TRACE_EXIT, "%s: ", __func__);
	return;
}

/*@}*/
