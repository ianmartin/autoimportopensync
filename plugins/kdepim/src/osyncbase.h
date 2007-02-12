#ifndef KDEPIM_OSYNC_BASE_H
#define KDEPIM_OSYNC_BASE_H

extern "C"
{
#include <opensync/opensync.h>
}
/** Base class to OpenSync plugin.
 *
 * This class is used mainly for avoid loading the KDE libraries only
 * when getting information about a plugin. The library that implements
 * the methods of this class will be loaded only when needed.
 */
class KdePluginImplementationBase
{
	public:
		virtual void connect(OSyncContext *ctx) = 0;
		virtual void disconnect(OSyncContext *ctx) = 0;

		virtual void get_changeinfo(OSyncContext *ctx) = 0;

		virtual void sync_done(OSyncContext *ctx) = 0;

		virtual bool vcard_access(OSyncContext *ctx, OSyncChange *chg) = 0;
		virtual bool vcard_commit_change(OSyncContext *ctx, OSyncChange *chg) = 0;

		virtual bool event_access(OSyncContext *ctx, OSyncChange *chg) = 0;
		virtual bool event_commit_change(OSyncContext *ctx, OSyncChange *chg) = 0;

		virtual bool todo_access(OSyncContext *ctx, OSyncChange *chg) = 0;
		virtual bool todo_commit_change(OSyncContext *ctx, OSyncChange *chg) = 0;

		virtual bool note_access(OSyncContext *ctx, OSyncChange *chg) = 0;
		virtual bool note_commit_change(OSyncContext *ctx, OSyncChange *chg) = 0;

		/* The declaration above seemed to be necessary just because the
		 * KdePluginImplementation destructor wasn't being called
		 */
		virtual ~KdePluginImplementationBase() { };
};

typedef KdePluginImplementationBase *(*KdeImplInitFunc)(OSyncMember *m, OSyncError **e);

#endif // KDEPIM_OSYNC_BASE_H
