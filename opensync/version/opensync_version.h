#ifndef OPENSYNC_VERSION_H_
#define OPENSYNC_VERSION_H_

OSYNC_EXPORT OSyncVersion *osync_version_new(OSyncError **error);
OSYNC_EXPORT void osync_version_ref(OSyncVersion *version);
OSYNC_EXPORT void osync_version_unref(OSyncVersion *version);

OSYNC_EXPORT char *osync_version_get_plugin(OSyncVersion *version);
OSYNC_EXPORT char *osync_version_get_priority(OSyncVersion *version);
OSYNC_EXPORT char *osync_version_get_modelversion(OSyncVersion *version);
OSYNC_EXPORT char *osync_version_get_firmwareversion(OSyncVersion *version);
OSYNC_EXPORT char *osync_version_get_softwareversion(OSyncVersion *version);
OSYNC_EXPORT char *osync_version_get_hardwareversion(OSyncVersion *version);
OSYNC_EXPORT char *osync_version_get_identifier(OSyncVersion *version);

OSYNC_EXPORT void osync_version_set_plugin(OSyncVersion *version, char *plugin);
OSYNC_EXPORT void osync_version_set_priority(OSyncVersion *version, char *priority);
OSYNC_EXPORT void osync_version_set_modelversion(OSyncVersion *version, char *modelversion);
OSYNC_EXPORT void osync_version_set_firmwareversion(OSyncVersion *version, char *firmwareversion);
OSYNC_EXPORT void osync_version_set_softwareversion(OSyncVersion *version, char *softwareversion);
OSYNC_EXPORT void osync_version_set_hardwareversion(OSyncVersion *version, char *hardwareversion);
OSYNC_EXPORT void osync_version_set_identifier(OSyncVersion *version, char *identifier);

OSYNC_EXPORT int osync_version_matches(OSyncVersion *pattern, OSyncVersion *version, OSyncError **error);
OSYNC_EXPORT OSyncList *osync_load_versions_from_descriptions(OSyncError **error);

#endif /*OPENSYNC_VERSION_H_*/
