#ifndef KDEPIM_OSYNC_BASE_H
#define KDEPIM_OSYNC_BASE_H

extern "C"
{
#include <opensync/opensync.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-context.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-helper.h>
#include <opensync/opensync-format.h>	
}

typedef struct OSyncKDEEnv {
	char *change_id;
	OSyncObjTypeSink *contact_sink;
} OSyncKDEEnv;
/** Base class to OpenSync plugin.
 *
 * This class is used mainly for avoid loading the KDE libraries only
 * when getting information about a plugin. The library that implements
 * the methods of this class will be loaded only when needed.
 */
class KdePluginImplementationBase
{
	public:
		virtual void connect(OSyncPluginInfo *info, OSyncContext *ctx) = 0;
		virtual void disconnect(OSyncPluginInfo *info, OSyncContext *ctx) = 0;

		virtual void get_changeinfo(OSyncPluginInfo *info, OSyncContext *ctx) = 0;

		virtual void sync_done(OSyncPluginInfo *info, OSyncContext *ctx) = 0;

		virtual bool vcard_access(OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *chg) = 0;
		virtual bool vcard_commit_change(OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *chg) = 0;


		/* The declaration above seemed to be necessary just because the
		 * KdePluginImplementation destructor wasn't being called
		 */
		virtual ~KdePluginImplementationBase() { };
};

typedef KdePluginImplementationBase *(*KdeImplInitFunc)(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error);

#endif // KDEPIM_OSYNC_BASE_H
