#ifndef OPENSYNC_VERSION_INTERNALS_H_
#define OPENSYNC_VERSION_INTERNALS_H_

/**
 * @brief Represent a Version object
 * @ingroup OSyncVersionPrivateAPI
 */
struct OSyncVersion{
	/** The reference counter for this object */
	int ref_count;
	char *plugin;
	char *priority;
	char *modelversion;
	char *firmwareversion;
	char *softwareversion;
	char *hardwareversion;
	char *identifier;
};

int _osync_version_match(char *pattern, char* string, OSyncError **error);

#endif /*OPENSYNC_VERSION_INTERNALS_H_*/
