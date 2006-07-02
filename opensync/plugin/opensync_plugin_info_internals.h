#ifndef OPENSYNC_PLUGIN_INFO_INTERNALS_H_
#define OPENSYNC_PLUGIN_INFO_INTERNALS_H_

struct OSyncPluginInfo {
	void *loop;
	char *config;
	GList *objtypes;
	char *configdir;
	OSyncObjTypeSink *sink;
	OSyncFormatEnv *formatenv;
	//devinfo
	int ref_count;
};

#endif /*OPENSYNC_PLUGIN_INFO_INTERNALS_H_*/
