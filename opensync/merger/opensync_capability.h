#ifndef OPENSYNC_CAPABILITY_H_
#define OPENSYNC_CAPABILITY_H_

OSyncCapability *osync_capability_new(OSyncCapability *capability, const char *name);

OSyncCapability *osync_capability_new_content_type(OSyncCapabilities *capabilities, const char *name);

void osync_capability_free(OSyncCapability *capability);

const char *osync_capability_get_name(OSyncCapability *capability);

OSyncCapability *osync_capability_get_next(OSyncCapability *capability);

OSyncCapability *osync_capability_get_first_child(OSyncCapability *capability);

int osync_capability_get_field_count(OSyncCapability *capability);

const char *osync_capability_get_nth_field(OSyncCapability *capability, int nth);

void osync_capability_add_Field(OSyncCapability *capabilitiy, const char *name);

#endif /*OPENSYNC_CAPABILITY_H_*/
