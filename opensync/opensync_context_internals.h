#ifndef _OPENSYNC_CONTEXT_INTERNALS_H_
#define _OPENSYNC_CONTEXT_INTERNALS_H_

struct OSyncContext {
	OSyncContextCallbackFn callback_function;
	void *callback_data;
	OSyncContextChangeFn changes_function;
	void *plugindata;
	OSyncMember *member;
	int ref_count;
};

#endif //_OPENSYNC_CONTEXT_INTERNALS_H_
