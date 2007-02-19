#include <opensync/opensync.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-context.h>
#include <opensync/opensync-helper.h>
#include <opensync/opensync-version.h>

#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "plugin.h"

static void free_env(plugin_environment *env)
{
	if (env->hashtable)
		osync_hashtable_free(env->hashtable);

	g_free(env);
}

static void connect(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, userdata, info, ctx);
	//Each time you get passed a context (which is used to track
	//calls to your plugin) you can get the data your returned in
	//initialize via this call:
	plugin_environment *env = (plugin_environment *)userdata;

	OSyncError *error = NULL;

	/*
	 * Now connect to your devices and report
	 * 
	 * an error via:
	 * osync_context_report_error(ctx, ERROR_CODE, "Some message");
	 * 
	 * or success via:
	 * osync_context_report_success(ctx);
	 * 
	 * You have to use one of these 2 somewhere to answer the context.
	 * 
	 */

	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);

	//If you need a hashtable you make it here
	char *tablepath = g_strdup_printf("%s/hashtable.db", osync_plugin_info_get_configdir(info));
	env->hashtable = osync_hashtable_new(tablepath, osync_objtype_sink_get_name(sink), &error);
	g_free(tablepath);

	if (!env->hashtable)
		goto error;

	//you can also use the anchor system to detect a device reset
	//or some parameter change here. Check the docs to see how it works
	char *lanchor = NULL;
	//Now you get the last stored anchor from the device
	char *anchorpath = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));

	if (!osync_anchor_compare(anchorpath, "lanchor", lanchor))
		osync_objtype_sink_set_slowsync(sink, TRUE);

	g_free(anchorpath);

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

static void get_changes(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
        osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, userdata, info, ctx);
        OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	plugin_environment *env = (plugin_environment *)userdata;

        OSyncError *error = NULL;

	//If you use opensync hashtables you can detect if you need
	//to do a slow-sync and set this on the hastable directly
	//otherwise you have to make 2 function like "get_changes" and
	//"get_all" and decide which to use using
	//osync_member_get_slow_sync
        if (osync_objtype_sink_get_slowsync(sink)) {
                osync_trace(TRACE_INTERNAL, "Slow sync requested");
                osync_hashtable_reset(env->hashtable);
        }

	/*
	 * Now you can get the changes.
	 * Loop over all changes you get and do the following:
	 */

	do {
		char *hash = g_strdup("<the calculated hash of the object>");
		char *uid = g_strdup("<some uid>");

		//Now get the data of this change
		char *data = NULL;

		OSyncChangeType changetype = osync_hashtable_get_changetype(env->hashtable, uid, hash);

		if (changetype == OSYNC_CHANGE_TYPE_UNMODIFIED) {
			g_free(hash);
			g_free(uid);
			continue;
		}


		//Set the hash of the object (optional, only required if you use hashtabled)
		osync_hashtable_update_hash(env->hashtable, changetype, uid, hash);

		//Make the new change to report
		OSyncChange *change = osync_change_new(&error);
		if (!change) {
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			continue;
		}

		//Now set the uid of the object
		osync_change_set_uid(change, uid);
		osync_change_set_hash(change, hash);
		osync_change_set_changetype(change, changetype);

		g_free(hash);

		OSyncData *odata = osync_data_new(data, 0, env->objformat, &error);
		if (!odata) {
			osync_change_unref(change);
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			continue;
		}

		osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));

		//Now you can set the data for the object
		osync_change_set_data(change, odata);
		osync_data_unref(odata);

		// just report the change via
		osync_context_report_change(ctx, change);

		osync_change_unref(change);

		g_free(uid);
	} while(0);

	//When you are done looping and if you are using hashtables
	//check for deleted entries ... via hashtable
	int i;
	char **uids = osync_hashtable_get_deleted(env->hashtable);
	for (i=0; uids[i]; i++) {
		OSyncChange *change = osync_change_new(&error);
		if (!change) {
			g_free(uids[i]);
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			continue;
		}

		osync_change_set_uid(change, uids[i]);
		osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_DELETED);

		OSyncData *odata = osync_data_new(NULL, 0, env->objformat, &error);
		if (!odata) {
			g_free(uids[i]);
			osync_change_unref(change);
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			continue;
		}

		osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));
		osync_change_set_data(change, odata);
		osync_data_unref(odata);

		osync_context_report_change(ctx, change);

		osync_hashtable_update_hash(env->hashtable, osync_change_get_changetype(change), osync_change_get_uid(change), NULL);

		osync_change_unref(change);
		g_free(uids[i]);
	}
	g_free(uids);

	//Now we need to answer the call
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void commit_change(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
	plugin_environment *env = (plugin_environment *)userdata;
	
	/*
	 * Here you have to add, modify or delete a object
	 * 
	 */
	switch (osync_change_get_changetype(change)) {
		case OSYNC_CHANGE_TYPE_DELETED:
			//Delete the change
			//Dont forget to answer the call on error
			break;
		case OSYNC_CHANGE_TYPE_ADDED:
			//Add the change
			//Dont forget to answer the call on error
			osync_change_set_hash(change, "new hash");
			break;
		case OSYNC_CHANGE_TYPE_MODIFIED:
			//Modify the change
			//Dont forget to answer the call on error
			osync_change_set_hash(change, "new hash");
			break;
		default:
			;
	}

	//If you are using hashtables you have to calculate the hash here:
	osync_hashtable_update_hash(env->hashtable, osync_change_get_changetype(change), osync_change_get_uid(change), "new hash");

	//Answer the call
	osync_context_report_success(ctx);
}

static void sync_done(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
//	plugin_environment *env = (plugin_environment *)userdata;
	
	/*
	 * This function will only be called if the sync was successful
	 */

	//If we use anchors we have to update it now.
	//Now you get/calculate the current anchor of the device
	char *lanchor = NULL;
	char *anchorpath = g_strdup_printf("%s/anchor.db", osync_plugin_info_get_configdir(info));
	osync_anchor_update(anchorpath, "lanchor", lanchor);
	g_free(anchorpath);
	
	//Answer the call
	osync_context_report_success(ctx);
}

static void disconnect(void *userdata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	plugin_environment *env = (plugin_environment *)userdata;
	
	//Close all stuff you need to close
	
	//Close the hashtable
	osync_hashtable_free(env->hashtable);
	env->hashtable = NULL;

	//Answer the call
	osync_context_report_success(ctx);
}

static void finalize(void *userdata)
{
	plugin_environment *env = (plugin_environment *)userdata;

	//Free all stuff that you have allocated here.
	free_env(env);
}


static void *initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	const char *configdata = NULL;
	
	//You need to specify the <some name>_environment somewhere with
	//all the members you need
	plugin_environment *env = osync_try_malloc0(sizeof(plugin_environment), error);
	if (!env)
		goto error;
	
	//now you can get the config file for this plugin
	configdata = osync_plugin_info_get_config(info);
	if (!configdata) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get config data.");
		goto error_free_env;
	}

	osync_trace(TRACE_INTERNAL, "The config: %s", osync_plugin_info_get_config(info));

	//Process the configdata here and set the options on your environment
	// ...
	
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	env->objformat = osync_format_env_find_objformat(formatenv, "<your format>");

	do {
		OSyncObjTypeSink *sink = osync_objtype_sink_new("<objtype>", error);
		if (!sink)
			goto error_free_env;

		osync_objtype_sink_add_objformat(sink, "<your format>");

		/* Every sink can have different functions ... */
		OSyncObjTypeSinkFunctions functions;
		memset(&functions, 0, sizeof(functions));
		functions.connect = connect;
		functions.disconnect = disconnect;
		functions.get_changes = get_changes;
		functions.commit = commit_change;
		functions.sync_done = sync_done;

		/* We pass the OSyncFileDir object to the sink, so we dont have to look it up
		 * again once the functions are called */
		osync_objtype_sink_set_functions(sink, functions, env);
		osync_plugin_info_add_objtype(info, sink);

	} while(0);
	
	//Now your return your struct.
	return (void *) env;

error_free_env:
        free_env(env);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

/* Here we actually tell opensync which sinks are available. */
static osync_bool discover(void *userdata, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, userdata, info, error);

//	plugin_environment *env = (plugin_environment *)userdata;

	// Report avaliable sinks...
	do {
		osync_objtype_sink_set_available(NULL, TRUE);
	} while(0);

	OSyncVersion *version = osync_version_new(error);
	osync_version_set_plugin(version, "<your plugin-name>");
	//osync_version_set_modelversion(version, "version");
	//osync_version_set_firmwareversion(version, "firmwareversion");
	//osync_version_set_softwareversion(version, "softwareversion");
	//osync_version_set_hardwareversion(version, "hardwareversion");
	osync_plugin_info_set_version(info, version);
	osync_version_unref(version);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}


osync_bool get_sync_info(OSyncPluginEnv *env, OSyncError **error)
{
	//Now you can create a new plugin information and fill in the details
	//Note that you can create several plugins here
	OSyncPlugin *plugin = osync_plugin_new(error);
	if (!plugin)
		goto error;
	
	//Tell opensync something about your plugin
	osync_plugin_set_name(plugin, "short name, maybe < 15 chars");
	osync_plugin_set_longname(plugin, "long name. maybe < 50 chars");
	osync_plugin_set_description(plugin, "A longer description. < 200 chars");

	//Now set the function we made earlier
	osync_plugin_set_initialize(plugin, initialize);
	osync_plugin_set_finalize(plugin, finalize);
	osync_plugin_set_discover(plugin, discover);

	osync_plugin_env_register_plugin(env, plugin);
	osync_plugin_unref(plugin);

	return TRUE;
error:
	osync_trace(TRACE_ERROR, "Unable to register: %s", osync_error_print(error));
	osync_error_unref(error);
	return FALSE;
}

int get_version(void)
{
	return 1;
}
