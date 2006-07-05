#ifndef OPENSYNC_CAPABILITIES_H_
#define OPENSYNC_CAPABILITIES_H_

OSYNC_EXPORT OSyncCapabilities *osync_capabilities_new(void);
OSYNC_EXPORT OSyncCapabilities *osync_capabilities_parse(const char *buffer, unsigned int size, OSyncError **error);
OSYNC_EXPORT void osync_capabilities_ref(OSyncCapabilities *capabilities);
OSYNC_EXPORT void osync_capabilities_unref(OSyncCapabilities *capabilities);

OSYNC_EXPORT OSyncCapability *osync_capabilities_get_first(OSyncCapabilities *capabilities, const char *objtype);
OSYNC_EXPORT osync_bool osync_capabilities_assemble(OSyncCapabilities *capabilities, char **buffer, int *size);
OSYNC_EXPORT void osync_capabilities_sort(OSyncCapabilities *capabilities);

#endif /*OPENSYNC_CAPABILITIES_H_*/
