#ifndef OPENSYNC_PLUGIN_INFO_INTERNALS_H_
#define OPENSYNC_PLUGIN_INFO_INTERNALS_H_

struct OSyncPluginInfo {
	void *loop;
	char *config;
	GList *objtypes;
	char *configdir;
	OSyncObjTypeSink *sink;
	OSyncObjTypeSink *current_sink;
	OSyncFormatEnv *formatenv;
	//devinfo
	int ref_count;
	char *groupname;
	OSyncVersion *version;
	OSyncCapabilities *capabilities;

#ifdef OPENSYNC_UNITTESTS
	long long int memberid; // introduced only for testing purpose (mock-sync)
#endif	
};

#endif /*OPENSYNC_PLUGIN_INFO_INTERNALS_H_*/
