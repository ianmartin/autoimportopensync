%module opensync_impl

%{
#include "opensync.h"
%}

%include "opensync.h"
typedef struct {} OSyncEnv;
typedef struct {} OSyncPlugin;

%extend OSyncPlugin {
	OSyncPlugin(OSyncEnv *env) {
		OSyncPlugin *plugin = osync_plugin_new(env);
		return plugin;
	}
	
	~OSyncPlugin() {
		osync_plugin_free(self);
	}
	
	void name_set(int inp) {
		printf("Trying to set it to %i\n", inp);
	}
	
	int name_get() {
		return 5;
	}
};

%extend OSyncEnv {
	OSyncEnv() {
		OSyncEnv *env = osync_env_new();
		return env;
	}
	
	~OSyncEnv() {
		osync_env_free(self);
	}
	
	osync_bool initialize() {
		return osync_env_initialize(self, NULL);
	}
	
	osync_bool finalize() {
		return osync_env_finalize(self, NULL);
	}
	
	int num_plugins() {
		return osync_env_num_plugins(self);
	}

	OSyncPlugin *get_nth_plugin(int nth) {
		OSyncPlugin *plugin = osync_env_nth_plugin(self, nth);
		return plugin;
	}
};
