#ifndef _OPENSYNC_CONTEXT_INTERNALS_H_
#define _OPENSYNC_CONTEXT_INTERNALS_H_

/*! @brief Represent a context that can be used to track calls to a plugin */
struct OSyncContext {
	OSyncEngCallback callback_function;
	void *calldata;
	OSyncMember *member;
	osync_bool success;
};

#endif //_OPENSYNC_CONTEXT_INTERNALS_H_
