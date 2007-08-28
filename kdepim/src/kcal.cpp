/***********************************************************************
KCalendar support for OpenSync kdepim-sync plugin
Copyright (C) 2004 Conectiva S. A.
 
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation;
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY
CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES 
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 
ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS, 
COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS 
SOFTWARE IS DISCLAIMED.
*************************************************************************/
/**
 * @author Eduardo Pereira Habkost <ehabkost@conectiva.com.br>
 * @author Andrew Baumann <andrewb@cse.unsw.edu.au>
 */

#include "kcal.h"

#include <assert.h>
#include <kapplication.h>
#include <dcopclient.h>

bool KCalSharedResource::open(OSyncContext *ctx)
{
	if (refcount++ > 0) {
		assert(calendar);
		return true;
	}

	DCOPClient *dcopc = KApplication::kApplication()->dcopClient();
	if (!dcopc) {
		osync_context_report_error(ctx, OSYNC_ERROR_INITIALIZATION, "Unable to initialize dcop client");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to initialize dcop client", __func__);
		return false;
	}

	QString appId = dcopc->registerAs("opensync-kcal");

	//check if korganizer running, and return an error if it
	//is running
	if (dcopc->isApplicationRegistered("korganizer")) {
		osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, "KOrganizer is running. Please finish it");
		osync_trace(TRACE_EXIT_ERROR, "%s: KOrganizer is running", __func__);
		return false;
	}

	calendar = new KCal::CalendarResources(QString::fromLatin1( "UTC" ));
	if (!calendar) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Can't open KDE calendar");
		return false;
	}
#if KDE_IS_VERSION(3,3,0)
	/* On KDE 3.2, there was no readConfig() and load(): the data
	 * was loaded automatically on the CalendarResources() constructor
	 */
	calendar->readConfig();
	calendar->load();
#endif

//	osync_debug("kcal", 3, "Calendar: %d events", calendar->events().size());

	return true;
}

bool KCalSharedResource::close(OSyncContext *)
{
	/* Save the changes */
	calendar->save();
	
	if (--refcount > 0)
		return true;

	delete calendar;
	calendar = NULL;
	return true;
}
	
static QString calc_hash(const KCal::Incidence *e)
{
	QDateTime d = e->lastModified();
	if (!d.isValid()) {
		d = QDateTime::currentDateTime();
		//e->setLastModified(&d);
	}
	/*FIXME: not i18ned string */
	return d.toString();
}

/** Add or change a incidence on the calendar. This function
 * is used for events and to-dos
 */
bool KCalSharedResource::commit(OSyncContext *ctx, OSyncChange *chg)
{
	OSyncChangeType type = osync_change_get_changetype(chg);
//	osync_debug("kcal", 3, "%s", __FUNCTION__);
	switch (type) {
		case OSYNC_CHANGE_TYPE_DELETED: {
			KCal::Incidence *e = calendar->incidence(osync_change_get_uid(chg));
			if (!e) {
				osync_context_report_error(ctx, OSYNC_ERROR_FILE_NOT_FOUND, "Event not found while deleting");
				return false;
			}
			calendar->deleteIncidence(e);
			break;
		}
		case OSYNC_CHANGE_TYPE_ADDED:
		case OSYNC_CHANGE_TYPE_MODIFIED: {
			KCal::ICalFormat format;
	
			OSyncData *odata = osync_change_get_data(chg);

			char *databuf;
			//size_t databuf_size; 
			// osync_data_get_data requires an unsigned int which is not compatible with size_t on 64bit machines
			unsigned int databuf_size = 0; 
			osync_data_get_data(odata, &databuf, &databuf_size);

			/* First, parse to a temporary calendar, because
				* we should set the uid on the events
				*/

			KCal::CalendarLocal cal(QString::fromLatin1( "UTC" ));
			QString data = QString::fromUtf8(databuf, databuf_size);
			if (!format.fromString(&cal, data)) {
				osync_context_report_error(ctx, OSYNC_ERROR_CONVERT, "Couldn't import calendar data");
				return false;
			}

			KCal::Incidence *oldevt = calendar->incidence(osync_change_get_uid(chg));
			if (oldevt)
				calendar->deleteIncidence(oldevt);

			/* Add the events from the temporary calendar, setting the UID
				*
				* We iterate over the list, but it should have only one event.
				*/
			KCal::Incidence::List evts = cal.incidences();
			for (KCal::Incidence::List::ConstIterator i = evts.begin(); i != evts.end(); i++) {
				KCal::Incidence *e = (*i)->clone();
				if (type == OSYNC_CHANGE_TYPE_MODIFIED)
					e->setUid(osync_change_get_uid(chg));

/*				osync_debug("kcal", 3, "Writing incidence: uid: %s, summary: %s",
						(const char*)e->uid().local8Bit(), (const char*)e->summary().local8Bit()); */

				QString c_uid = e->uid().utf8();
				osync_change_set_uid(chg, (const char*)c_uid);
				QString hash = calc_hash(*i);
				osync_change_set_hash(chg, hash);
				calendar->addIncidence(e);
			}
			break;
		}
		default: {
			osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Invalid or unsupported change type");
			return false;
		}
	}

	return true;
}

/** Report a list of calendar incidences (events or to-dos), with the
 * right objtype and objformat.
 *
 * This function exists because the logic for converting the events or to-dos
 * is the same, only the objtype and format is different.
 */
bool KCalSharedResource::report_incidence(OSyncDataSource *dsobj, OSyncPluginInfo *info, OSyncContext *ctx, KCal::Incidence *e, OSyncObjFormat *objformat)
{
	/* Build a local calendar for the incidence data */
	KCal::CalendarLocal cal(calendar->timeZoneId());
//	osync_debug("kcal", 3, "timezoneid: %s\n", (const char*)cal.timeZoneId().local8Bit());
	cal.addIncidence(e->clone());

	/* Convert the data to vcalendar */
	KCal::ICalFormat format;
	QString data = format.toString(&cal);

	return dsobj->report_change(info, ctx, e->uid(), data, calc_hash(e), objformat);
}

bool KCalSharedResource::get_event_changes(OSyncDataSource *dsobj, OSyncPluginInfo *info, OSyncContext *ctx)
{
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	OSyncObjFormat *objformat = osync_format_env_find_objformat(formatenv, "vevent20");
	
	KCal::Event::List events = calendar->events();
//	osync_debug("kcal", 3, "Number of events: %d", events.size());

	for (KCal::Event::List::ConstIterator i = events.begin(); i != events.end(); i++) {

		/* Skip entries from birthday resource. This is just a workaround.
		 * patch by rhuitl
		 * FIXME: todo: add a list of resources to kdepim-sync.conf
		 */
		if ( (*i)->uid().contains("KABC_Birthday") || (*i)->uid().contains("KABC_Anniversary") )
			continue;

		if (!report_incidence(dsobj, info, ctx, *i, objformat))
			return false;
	}

	return true;
}

bool KCalSharedResource::get_todo_changes(OSyncDataSource *dsobj, OSyncPluginInfo *info, OSyncContext *ctx)
{
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	OSyncObjFormat *objformat = osync_format_env_find_objformat(formatenv, "vtodo20");
	
	KCal::Todo::List todos = calendar->todos();

//	osync_debug("kcal", 3, "Number of to-dos: %d", todos.size());

	for (KCal::Todo::List::ConstIterator i = todos.begin(); i != todos.end(); i++) {
//		osync_debug("kcal", 3, "%p: doesFloat: %d", *i, (*i)->doesFloat());
		if (!report_incidence(dsobj, info, ctx, *i, objformat))
			return false;
	}

	return true;
}

bool KCalEventDataSource::initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, plugin, info);

	if (!OSyncDataSource::initialize(plugin, info, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s", __PRETTY_FUNCTION__);
		return false;
	}

	osync_objtype_sink_add_objformat(sink, "vevent20");

	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
	return true;
}

bool KCalTodoDataSource::initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, plugin, info);

	if (!OSyncDataSource::initialize(plugin, info, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s", __PRETTY_FUNCTION__);
		return false;
	}

	osync_objtype_sink_add_objformat(sink, "vtodo20");

	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
	return true;
}

void KCalEventDataSource::connect(OSyncPluginInfo *info, OSyncContext *ctx)
{
	if (!kcal->open(ctx))
		return;
	OSyncDataSource::connect(info, ctx);
}

void KCalTodoDataSource::connect(OSyncPluginInfo *info, OSyncContext *ctx)
{
	if (kcal->open(ctx))
		OSyncDataSource::connect(info, ctx);
}

void KCalEventDataSource::disconnect(OSyncPluginInfo *, OSyncContext *ctx)
{
	if (kcal->close(ctx))
		osync_context_report_success(ctx);
}

void KCalTodoDataSource::disconnect(OSyncPluginInfo *, OSyncContext *ctx)
{
	if (kcal->close(ctx))
		osync_context_report_success(ctx);
}

void KCalEventDataSource::get_changes(OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, info, ctx);

	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);

	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	OSyncObjFormat *objformat = osync_format_env_find_objformat(formatenv, "vevent20");

	if (osync_objtype_sink_get_slowsync(sink)) {
		osync_trace(TRACE_INTERNAL, "Got slow-sync");
		osync_hashtable_reset(hashtable);
	}

	if (!kcal->get_event_changes(this, info, ctx)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: error in get_todo_changes", __PRETTY_FUNCTION__);
		return;
	}

	if (!report_deleted(info, ctx, objformat)) {
		osync_trace(TRACE_EXIT_ERROR, "%s", __PRETTY_FUNCTION__);
		return;
	}

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

void KCalTodoDataSource::get_changes(OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, info, ctx);

	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);

	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	OSyncObjFormat *objformat = osync_format_env_find_objformat(formatenv, "vtodo20");

	if (osync_objtype_sink_get_slowsync(sink)) {
		osync_trace(TRACE_INTERNAL, "Got slow-sync");
		osync_hashtable_reset(hashtable);
	}

	if (!kcal->get_todo_changes(this, info, ctx)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: error in get_todo_changes", __PRETTY_FUNCTION__);
		return;
	}

	if (!report_deleted(info, ctx, objformat)) {
		osync_trace(TRACE_EXIT_ERROR, "%s", __PRETTY_FUNCTION__);
		return;
	}

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

void KCalEventDataSource::commit(OSyncPluginInfo *, OSyncContext *ctx, OSyncChange *chg)
{
	// We use the same function for events and to-do
	if (!kcal->commit(ctx, chg))
		return;
	
	osync_hashtable_update_hash(hashtable, osync_change_get_changetype(chg), osync_change_get_uid(chg), osync_change_get_hash(chg));
	osync_context_report_success(ctx);
}

void KCalTodoDataSource::commit(OSyncPluginInfo *, OSyncContext *ctx, OSyncChange *chg)
{
	// We use the same function for calendar and to-do
	if (!kcal->commit(ctx, chg))
		return;
	
	osync_hashtable_update_hash(hashtable, osync_change_get_changetype(chg), osync_change_get_uid(chg), osync_change_get_hash(chg));
	osync_context_report_success(ctx);
}
