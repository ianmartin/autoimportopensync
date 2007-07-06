#ifndef KDEPIM_OSYNC_DATASOURCE_BASE_H
#define KDEPIM_OSYNC_DATASOURCE_BASE_H

/* abstract parent class for all KDE Data sources/sinks */
class OSyncDataSourceBase
{
	public:
		virtual ~OSyncDataSourceBase() {};

		virtual bool initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error) = 0;

		virtual void connect(OSyncPluginInfo *info, OSyncContext *ctx) = 0;
		virtual void disconnect(OSyncPluginInfo *info, OSyncContext *ctx) = 0;
		virtual void get_changes(OSyncPluginInfo *info, OSyncContext *ctx) = 0;
		virtual bool read(OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *chg) = 0;
		virtual void commit(OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *chg) = 0;
		virtual void sync_done(OSyncPluginInfo *info, OSyncContext *ctx) = 0;
};

/* common implementation of OSyncDataSourceBase */
class OSyncDataSource : public OSyncDataSourceBase
{
	protected:
		const char *objtype;
		OSyncHashTable *hashtable;
		OSyncObjTypeSink *sink;

	public:
		OSyncDataSource(const char *objtype) : objtype(objtype) {}
		virtual ~OSyncDataSource();
		
		virtual bool initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error);
		virtual void connect(OSyncPluginInfo *info, OSyncContext *ctx);
		virtual void sync_done(OSyncPluginInfo *info, OSyncContext *ctx);
};

#endif // KDEPIM_OSYNC_DATASOURCE_BASE_H
