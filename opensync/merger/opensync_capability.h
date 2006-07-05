#ifndef OPENSYNC_CAPABILITY_H_
#define OPENSYNC_CAPABILITY_H_

OSYNC_EXPORT OSyncCapability *osync_capability_new(OSyncCapabilities *capabilities, const char *objtype, const char *name);

OSYNC_EXPORT const char *osync_capability_get_name(OSyncCapability *capability);
OSYNC_EXPORT OSyncCapability *osync_capability_get_next(OSyncCapability *capability);

OSYNC_EXPORT osync_bool osync_capability_has_key(OSyncCapability *capability);
OSYNC_EXPORT int osync_capability_get_key_count(OSyncCapability *capability);
OSYNC_EXPORT const char *osync_capability_get_nth_key(OSyncCapability *capability, int nth);
OSYNC_EXPORT void osync_capability_add_key(OSyncCapability *capabilitiy, const char *name);

#endif /*OPENSYNC_CAPABILITY_H_*/
