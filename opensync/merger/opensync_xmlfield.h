#ifndef OPENSYNC_XMLFIELD_H_
#define OPENSYNC_XMLFIELD_H_

OSYNC_EXPORT OSyncXMLField *osync_xmlfield_new(OSyncXMLFormat *xmlformat, const char *name);
OSYNC_EXPORT void osync_xmlfield_free(OSyncXMLField *xmlfield);

OSYNC_EXPORT const char *osync_xmlfield_get_name(OSyncXMLField *xmlfield);
OSYNC_EXPORT OSyncXMLField *osync_xmlfield_get_next(OSyncXMLField *xmlfield);

OSYNC_EXPORT const char *osync_xmlfield_get_attr(OSyncXMLField *xmlfield, const char *attr);
OSYNC_EXPORT void osync_xmlfield_set_attr(OSyncXMLField *xmlfield, const char *attr, const char *value);
OSYNC_EXPORT int osync_xmlfield_get_attr_count(OSyncXMLField *xmlfield);
OSYNC_EXPORT const char *osync_xmlfield_get_nth_attr_name(OSyncXMLField *xmlfield, int nth);
OSYNC_EXPORT const char *osync_xmlfield_get_nth_attr_value(OSyncXMLField *xmlfield, int nth);

OSYNC_EXPORT const char *osync_xmlfield_get_key_value(OSyncXMLField *xmlfield, const char *key);
OSYNC_EXPORT void osync_xmlfield_set_key_value(OSyncXMLField *xmlfield, const char *key, const char *value);
OSYNC_EXPORT void osync_xmlfield_add_key_value(OSyncXMLField *xmlfield, const char *key, const char *value);
OSYNC_EXPORT int osync_xmlfield_get_key_count(OSyncXMLField *xmlfield);
OSYNC_EXPORT const char *osync_xmlfield_get_nth_key_name(OSyncXMLField *xmlfield, int nth);
OSYNC_EXPORT const char *osync_xmlfield_get_nth_key_value(OSyncXMLField *xmlfield, int nth);
OSYNC_EXPORT void osync_xmlfield_set_nth_key_value(OSyncXMLField *xmlfield, int nth, const char *value);

OSYNC_EXPORT void osync_xmlfield_unlink(OSyncXMLField *xmlfield);
OSYNC_EXPORT void osync_xmlfield_link_before_field(OSyncXMLField *xmlfield, OSyncXMLField *to_link);
OSYNC_EXPORT void osync_xmlfield_link_after_field(OSyncXMLField *xmlfield, OSyncXMLField *to_link);

OSYNC_EXPORT osync_bool osync_xmlfield_compare(OSyncXMLField *xmlfield1, OSyncXMLField *xmlfield2);
OSYNC_EXPORT osync_bool osync_xmlfield_compare_similar(OSyncXMLField *xmlfield1, OSyncXMLField *xmlfield2, char* keys[]);
OSYNC_EXPORT int osync_xmlfield_compare_stdlib(const void *xmlfield1, const void *xmlfield2);

//OSYNC_EXPORT OSyncXMLField *osync_xmlfield_insert_copy_before_field(OSyncXMLField *xmlfield, OSyncXMLField *to_copy);

#endif /*OPENSYNC_XMLFIELD_H_*/
