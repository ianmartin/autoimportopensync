#ifndef _OPENSYNC_CONTEXT_INTERNALS_H_
#define _OPENSYNC_CONTEXT_INTERNALS_H_

#ifndef DOXYGEN_SHOULD_SKIP_THIS
struct OSyncContext {
	OSyncEngCallback callback_function;
	void *calldata;
	OSyncMember *member;
	osync_bool success;
};
#endif

#endif //_OPENSYNC_CONTEXT_INTERNALS_H_
