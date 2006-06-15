#ifndef OPENSYNC_XMLFIELD_H_
#define OPENSYNC_XMLFIELD_H_

/* the returned OSyncXMLField will be freed with the given OSyncXMLFormat Object */
OSyncXMLField *osync_xmlfield_new(OSyncXMLFormat *xmlformat, const char *name);

void osync_xmlfield_free(OSyncXMLField *xmlfield);

OSyncXMLField *osync_xmlfield_get_next(OSyncXMLField *xmlfield);

const char *osync_xmlfield_get_name(OSyncXMLField *xmlfield);


const char *osync_xmlfield_get_attr(OSyncXMLField *xmlfield, const char *attr);

void osync_xmlfield_set_attr(OSyncXMLField *xmlfield, const char *attr, const char *value);

int osync_xmlfield_get_attr_count(OSyncXMLField *xmlfield);

const char *osync_xmlfield_get_nth_attr_value(OSyncXMLField *xmlfield, int nth);

void osync_xmlfield_set_nth_key_value(OSyncXMLField *xmlfield, int nth, const char *value);


const char *osync_xmlfield_get_key_value(OSyncXMLField *xmlfield, const char *key);

void osync_xmlfield_set_key_value(OSyncXMLField *xmlfield, const char *key, const char *value);

int osync_xmlfield_get_key_count(OSyncXMLField *xmlfield);

const char *osync_xmlfield_get_nth_key_value(OSyncXMLField *xmlfield, int nth);

void osync_xmlfield_set_nth_key_value(OSyncXMLField *xmlfield, int nth, const char *value);


void osync_xmlfield_unlink(OSyncXMLField *xmlfield);

void osync_xmlfield_link_before_field(OSyncXMLField *xmlfield, OSyncXMLField *to_link);

void osync_xmlfield_link_after_field(OSyncXMLField *xmlfield, OSyncXMLField *to_link);

int osync_xmlfield_compaire(const void *xmlfield1, const void *xmlfield2);

/* not needed anymore */
OSyncXMLField *osync_xmlfield_insert_copy_before_field(OSyncXMLField *xmlfield, OSyncXMLField *to_copy);

#endif /*OPENSYNC_XMLFIELD_H_*/
