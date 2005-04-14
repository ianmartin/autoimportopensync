%module opensync

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
	
	%pythoncode %{
	def get_name(self):
		return self.name_get()
	def set_name(self, name):
		self.name_set(name)
	name = property(get_name, set_name)
	%}
	
	const char *name_get() {
		return osync_plugin_get_name(self);
	}
};

%extend OSyncEnv {
	OSyncEnv(OSyncEnv *exenv) {
		if (exenv)
			return exenv;
		OSyncEnv *env = osync_env_new();
		return env;
	}
	
	~OSyncEnv() {
		osync_env_free(self);
	}
	
	int initialize() {
		return osync_env_initialize(self, NULL);
	}
	
	int finalize() {
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
