#ifndef OPENSYNC_CAPABILITIES_H_
#define OPENSYNC_CAPABILITIES_H_

OSyncCapabilities *osync_capabilities_new(void);
void osync_capabilities_free(OSyncCapabilities *capabilities);
void osync_capabilities_ref(OSyncCapabilities *capabilities);
void osync_capabilities_unref(OSyncCapabilities *capabilities);
OSyncCapability *osync_capabilities_get_first(OSyncCapabilities *capabilities);
osync_bool osync_capabilities_read_xml(OSyncCapabilities *capabilities, const char *path, OSyncError **error);
void osync_capabilities_write_xml(OSyncCapabilities *capabilities, const char *path, OSyncError **error);
OSyncCapability *osync_capabilities_getFirst(OSyncCapabilities *capabilities);
void osync_capabilities_sort(OSyncCapabilities *capabilities);

#endif /*OPENSYNC_CAPABILITIES_H_*/
