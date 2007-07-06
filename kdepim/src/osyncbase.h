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

/** Base class to OpenSync plugin.
 *
 * This class is used mainly for avoid loading the KDE libraries only
 * when getting information about a plugin. The library that implements
 * the methods of this class will be loaded only when needed.
 */
class KdePluginImplementationBase
{
	public:
		virtual ~KdePluginImplementationBase() {};
};

typedef KdePluginImplementationBase *(*KdeImplInitFunc)(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error);

#endif // KDEPIM_OSYNC_BASE_H
