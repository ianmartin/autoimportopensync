%module opensync

%{
#include "opensync.h"
%}

%include "opensync.h"
typedef struct {
} OSyncEnv;

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
};
