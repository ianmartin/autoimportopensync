#ifndef OPENSYNC_CAPABILITIES_H_
#define OPENSYNC_CAPABILITIES_H_

OSYNC_EXPORT OSyncCapabilities *osync_capabilities_new(OSyncError **error);
OSYNC_EXPORT OSyncCapabilities *osync_capabilities_parse(const char *buffer, unsigned int size, OSyncError **error);
OSYNC_EXPORT void osync_capabilities_ref(OSyncCapabilities *capabilities);
OSYNC_EXPORT void osync_capabilities_unref(OSyncCapabilities *capabilities);

OSYNC_EXPORT OSyncCapability *osync_capabilities_get_first(OSyncCapabilities *capabilities, const char *objtype);
OSYNC_EXPORT osync_bool osync_capabilities_assemble(OSyncCapabilities *capabilities, char **buffer, int *size);
OSYNC_EXPORT void osync_capabilities_sort(OSyncCapabilities *capabilities);

OSYNC_EXPORT OSyncCapabilities *osync_capabilities_load(const char *file, OSyncError **error);
OSYNC_EXPORT osync_bool osync_capabilities_member_has_capabilities(OSyncMember *member);
OSYNC_EXPORT OSyncCapabilities* osync_capabilities_member_get_capabilities(OSyncMember *member, OSyncError** error);
OSYNC_EXPORT osync_bool osync_capabilities_member_set_capabilities(OSyncMember *member, OSyncCapabilities* capabilities, OSyncError** error);

#endif /*OPENSYNC_CAPABILITIES_H_*/
