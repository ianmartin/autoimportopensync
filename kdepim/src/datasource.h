#ifndef KDEPIM_OSYNC_DATASOURCE_H
#define KDEPIM_OSYNC_DATASOURCE_H

/* common parent class and shared code for all KDE Data sources/sinks */
class OSyncDataSource
{
	friend class KCalSharedResource;

	protected:
		const char *objtype;
		OSyncHashTable *hashtable;
		OSyncObjTypeSink *sink;

		/* utility functions for subclasses */
		bool report_change(OSyncPluginInfo *info, OSyncContext *ctx, QString uid, QString data, QString hash, OSyncObjFormat *objformat);
		bool report_deleted(OSyncPluginInfo *info, OSyncContext *ctx);

	public:
		OSyncDataSource(const char *objtype) : objtype(objtype) {}
		virtual ~OSyncDataSource();
		
		/* common code for some of the callbacks, should be used by a subclass */
		bool initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error);

		/* these map to opensync's per-sink callbacks */
		virtual void connect(OSyncPluginInfo *info, OSyncContext *ctx);
		virtual void disconnect(OSyncPluginInfo *info, OSyncContext *ctx) = 0;
		virtual void get_changes(OSyncPluginInfo *info, OSyncContext *ctx) = 0;
		virtual void commit(OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *chg) = 0;
		virtual void sync_done(OSyncPluginInfo *info, OSyncContext *ctx);
};

#endif // KDEPIM_OSYNC_DATASOURCE_H
