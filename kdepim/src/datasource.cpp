/**
 * @author Andrew Baumann <andrewb@cse.unsw.edu.au>
 */

#include <stdlib.h>
#include <kapplication.h>

#include "osyncbase.h"
#include "datasource.h"

extern "C"
{
/* NB: the userdata passed to the sink callbacks is not the sink's userdata, as
 * you might expect, but instead the plugin's userdata, which in our case is the
 * KdePluginImplementation object pointer. So we need to get the sink object from
 * osync_objtype_sink_get_userdata(osync_plugin_info_get_sink(info))
 */

static void connect_wrapper(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __PRETTY_FUNCTION__, userdata, info, ctx);
	OSyncDataSource *obj = (OSyncDataSource *)osync_objtype_sink_get_userdata(osync_plugin_info_get_sink(info));
	obj->connect(info, ctx);
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

static void disconnect_wrapper(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __PRETTY_FUNCTION__, userdata, info, ctx);
	OSyncDataSource *obj = (OSyncDataSource *)osync_objtype_sink_get_userdata(osync_plugin_info_get_sink(info));
	obj->disconnect(info, ctx);
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

static void get_changes_wrapper(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __PRETTY_FUNCTION__, userdata, info, ctx);
	OSyncDataSource *obj = (OSyncDataSource *)osync_objtype_sink_get_userdata(osync_plugin_info_get_sink(info));
	obj->get_changes(info, ctx);
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

static void commit_wrapper(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *chg)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __PRETTY_FUNCTION__, userdata, info, ctx, chg);
	OSyncDataSource *obj = (OSyncDataSource *)osync_objtype_sink_get_userdata(osync_plugin_info_get_sink(info));
	obj->commit(info, ctx, chg);
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

static void sync_done_wrapper(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __PRETTY_FUNCTION__, userdata, info, ctx);
	OSyncDataSource *obj = (OSyncDataSource *)osync_objtype_sink_get_userdata(osync_plugin_info_get_sink(info));
	obj->sync_done(info, ctx);
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

} // extern "C"

/* Warning: this struct initialisation depends on the order of the fields in 
 * OSyncObjTypeSinkFunctions, but C++ doesn't allow C99-style named structure
 * initialisation, and I can't figure out any other way to do it */
static OSyncObjTypeSinkFunctions wrapper_functions = {
	connect_wrapper,
	disconnect_wrapper,
	get_changes_wrapper,
	commit_wrapper,
	NULL, // write
	NULL, // committed_all
	NULL, // read
	NULL, // batch_commit
	sync_done_wrapper};

bool OSyncDataSource::initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, plugin, info);
	
	sink = osync_objtype_sink_new(objtype, error);
	if (sink == NULL) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __PRETTY_FUNCTION__, osync_error_print(error));
		return false;
	}

	osync_objtype_sink_set_functions(sink, wrapper_functions, this);
	osync_plugin_info_add_objtype(info, sink);

	const char *configdir = osync_plugin_info_get_configdir(info);
	QString tablepath = QString("%1/%2-hash.db").arg(configdir, objtype);
	hashtable = osync_hashtable_new(tablepath, osync_objtype_sink_get_name(sink), error);
	if (hashtable == NULL) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __PRETTY_FUNCTION__, osync_error_print(error));
		return false;
	}

	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
	return true;
}

void OSyncDataSource::connect(OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, info, ctx);

	//Detection mechanismn if this is the first sync
	QString anchorpath = QString("%1/anchor.db").arg(osync_plugin_info_get_configdir(info));
	if (!osync_anchor_compare(anchorpath, objtype, "true")) {
		osync_trace(TRACE_INTERNAL, "Setting slow-sync for %s", objtype);
		osync_objtype_sink_set_slowsync(sink, TRUE);
	}
	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

void OSyncDataSource::sync_done(OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, info, ctx);

	//Detection mechanismn if this is the first sync
	QString anchorpath = QString("%1/anchor.db").arg(osync_plugin_info_get_configdir(info));
	osync_anchor_update(anchorpath, objtype, "true");
	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

bool OSyncDataSource::report_change(OSyncPluginInfo *info, OSyncContext *ctx, QString uid, QString data, QString hash, OSyncObjFormat *objformat)
{
	OSyncError *error = NULL;
	OSyncChangeType changetype;
	OSyncData *odata;
	OSyncChange *change;
	char *data_str;

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %s, (data), (hash), %p)", __PRETTY_FUNCTION__, info, ctx, uid.data(), objformat);

	change = osync_change_new(&error);
	if (!change)
		goto error;

	osync_change_set_uid(change, uid.local8Bit());

	data_str = strdup((const char *)data.utf8());

	osync_trace(TRACE_SENSITIVE,"Data:\n%s", data_str);

	odata = osync_data_new(data_str, strlen(data_str), objformat, &error);
	if (!odata)
		goto error;

	osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));

	//Now you can set the data for the object
	osync_change_set_data(change, odata);
	osync_data_unref(odata);

	// Use the hash table to check if the object
	// needs to be reported
	osync_change_set_hash(change, hash.data());

	// Report entry ... otherwise it gets deleted!
	osync_hashtable_report(hashtable, uid);

	changetype = osync_hashtable_get_changetype(hashtable, uid, hash.data());
	osync_change_set_changetype(change, changetype);
	if (OSYNC_CHANGE_TYPE_UNMODIFIED != changetype) {
		osync_context_report_change(ctx, change);
		osync_hashtable_update_hash(hashtable, changetype, uid, hash.data());
	}

	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
	return true;

error:
	if (change)
		osync_change_unref(change);
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __PRETTY_FUNCTION__, osync_error_print(&error));
	osync_error_unref(&error);
	return false;
}

bool OSyncDataSource::report_deleted(OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, info, ctx);
	
	int i;
	OSyncError *error = NULL;
	char **uids = osync_hashtable_get_deleted(hashtable);
	OSyncChange *change;
	
	for (i=0; uids[i]; i++) {
		osync_trace(TRACE_INTERNAL, "going to delete entry with uid: %s", uids[i]);
		
		change = osync_change_new(&error);
		if (!change) {
			for (; uids[i]; i++)
				g_free(uids[i]);
			g_free(uids);
			osync_context_report_osyncerror(ctx, error);
			osync_trace(TRACE_EXIT_ERROR, "%s: %s", __PRETTY_FUNCTION__, osync_error_print(&error));
			osync_error_unref(&error);
			return false;
		}

		osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_DELETED);
		osync_change_set_uid(change, uids[i]);
		osync_context_report_change(ctx, change);
		osync_hashtable_update_hash(hashtable, OSYNC_CHANGE_TYPE_DELETED, uids[i], NULL);

		g_free(uids[i]);
		osync_change_unref(change);
	}
	g_free(uids);
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
	return true;
}

OSyncDataSource::~OSyncDataSource()
{
	if (sink)
		osync_objtype_sink_unref(sink);

	if (hashtable)
		osync_hashtable_free(hashtable);
}
