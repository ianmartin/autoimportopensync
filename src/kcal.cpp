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
 * @autor Eduardo Pereira Habkost <ehabkost@conectiva.com.br>
 */

#include <libkcal/calendarresources.h>
#include <libkcal/icalformat.h>
#include <libkcal/calendarlocal.h>
#include <kdeversion.h>

#include "osyncbase.h"
#include "kcal.h"

KCalDataSource::KCalDataSource(OSyncMember *member, OSyncHashTable *hashtable)
    : hashtable(hashtable), member(member)
{
}

bool KCalDataSource::connect(OSyncContext *ctx)
{
    calendar = new KCal::CalendarResources();
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
    osync_debug("kcal", 3, "Calendar: %d events", calendar->events().size());
    return true;
}

bool KCalDataSource::disconnect(OSyncContext *)
{
	
    /* Save the changes */
	calendar->save();
    
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

/** Report a list of calendar incidences (events or to-dos), with the
 * right objtype and objformat.
 *
 * This function exists because the logic for converting the events or to-dos
 * is the same, only the objtype and format is different.
 */
bool KCalDataSource::report_incidence(OSyncContext *ctx, KCal::Incidence *e, const char *objtype, const char *objformat)
{
	osync_debug("kcal", 3, "One calendar incidence (%s)", objtype);
	QString hash = calc_hash(e);

	QString uid = e->uid();

	/* Build a local calendar for the incidence data */
	KCal::CalendarLocal cal(calendar->timeZoneId());
	osync_debug("kcal", 3, "timezoneid: %s\n", (const char*)cal.timeZoneId().local8Bit());
	cal.addIncidence(e->clone());

	/* Convert the data to vcalendar */
	KCal::ICalFormat format;
	QCString datastr = format.toString(&cal).utf8();
	const char *data = datastr;

	osync_debug("kcal", 3, "UID: %s\n", (const char*)uid.local8Bit());
	OSyncChange *chg = osync_change_new();
	osync_change_set_uid(chg, uid.local8Bit());
	osync_change_set_member(chg, member);

	// object type and format
	osync_change_set_objtype_string(chg, objtype);
	osync_change_set_objformat_string(chg, objformat);
	osync_change_set_data(chg, strdup(data), strlen(data) +1, 1);

	// Use the hash table to check if the object
	// needs to be reported
	osync_change_set_hash(chg, hash.data());
	if (osync_hashtable_detect_change(hashtable, chg)) {
		osync_context_report_change(ctx, chg);
		osync_hashtable_update_hash(hashtable, chg);
	}

    return true;
}

bool KCalDataSource::get_changeinfo_events(OSyncContext *ctx)
{
    KCal::Event::List events = calendar->events();
    osync_debug("kcal", 3, "Number of events: %d", events.size());

	if (osync_member_get_slow_sync(member, "event")) {
		osync_debug("kcal", 3, "Setting slow-sync for events");
		osync_hashtable_set_slow_sync(hashtable, "event");
	}
	
    for (KCal::Event::List::ConstIterator i = events.begin(); i != events.end(); i++) {
        if (!report_incidence(ctx, *i, "event", "vevent20"))
            return false;
    }

    osync_hashtable_report_deleted(hashtable, ctx, "event");

    return true;
}

bool KCalDataSource::get_changeinfo_todos(OSyncContext *ctx)
{
    KCal::Todo::List todos = calendar->todos();

    osync_debug("kcal", 3, "Number of to-dos: %d", todos.size());
    
    if (osync_member_get_slow_sync(member, "todo")) {
		osync_debug("kcal", 3, "Setting slow-sync for todos");
		osync_hashtable_set_slow_sync(hashtable, "todo");
	}
    
    for (KCal::Todo::List::ConstIterator i = todos.begin(); i != todos.end(); i++) {
        osync_debug("kcal", 3, "%p: doesFloat: %d", *i, (*i)->doesFloat());
        if (!report_incidence(ctx, *i, "todo", "vtodo20"))
            return false;
    }

    osync_hashtable_report_deleted(hashtable, ctx, "todo");

    return true;

}

void KCalDataSource::get_data(OSyncContext *ctx, OSyncChange *)
{
    osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Not implemented yet");
}

/** Add or change a incidence on the calendar. This function
 * is used for events and to-dos
 */
bool KCalDataSource::__access(OSyncContext *ctx, OSyncChange *chg)
{
    OSyncChangeType type = osync_change_get_changetype(chg);
    osync_debug("kcal", 3, "%s", __FUNCTION__);
    switch (type) {
        case CHANGE_DELETED:
            {
                KCal::Incidence *e = calendar->incidence(osync_change_get_uid(chg));
                if (!e) {
                    osync_context_report_error(ctx, OSYNC_ERROR_FILE_NOT_FOUND, "Event not found while deleting");
                    return false;
                }
                calendar->deleteIncidence(e);
            }
        break;
        case CHANGE_ADDED:
        case CHANGE_MODIFIED:
        {
            KCal::ICalFormat format;

            /* First, parse to a temporary calendar, because
             * we should set the uid on the events
             */
            KCal::CalendarLocal cal;
            QString data = QString::fromUtf8(osync_change_get_data(chg), osync_change_get_datasize(chg));
            if (!format.fromString(&cal, data)) {
                osync_context_report_error(ctx, OSYNC_ERROR_CONVERT, "Couldn't import calendar data");
                return false;
            }

            /*FIXME: The event/to-do will be overwritten. But I can't differentiate
             * between a field being removed and a missing field because
             * the other device don't support them, because OpenSync currently
             * doesn't have support for it. Yes, it is risky, but unfortunately
             * CalendarResources don't have support for changing an event
             * from vcard data, and not adding it.
             */
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
                if (type == CHANGE_MODIFIED)
               		e->setUid(osync_change_get_uid(chg));

                osync_debug("kcal", 3, "Writing incidence: uid: %s, summary: %s",
                                (const char*)e->uid().local8Bit(), (const char*)e->summary().local8Bit());
                                
                QString c_uid = e->uid().utf8();
                osync_change_set_uid(chg, (const char*)c_uid);
                QString hash = calc_hash(*i);
                osync_change_set_hash(chg, hash);
                calendar->addIncidence(e);
            }


        }
        break;
        default:
            osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Invalid or unsupported change type");
            return false;
    }

    return true;
}

bool KCalDataSource::event_access(OSyncContext *ctx, OSyncChange *chg)
{
    // We use the same function for events and to-do
    if (!__access(ctx, chg))
        return false;

    osync_context_report_success(ctx);
    return true;
}

bool KCalDataSource::event_commit_change(OSyncContext *ctx, OSyncChange *chg)
{
    // We use the same function for events and to-do
    if ( !__access(ctx, chg) )
        return false;
    osync_hashtable_update_hash(hashtable, chg);
    osync_context_report_success(ctx);
    return true;
}

bool KCalDataSource::todo_access(OSyncContext *ctx, OSyncChange *chg)
{
    // We use the same function for calendar and to-do
    if (!__access(ctx, chg))
        return false;

    osync_context_report_success(ctx);
    return true;
}

bool KCalDataSource::todo_commit_change(OSyncContext *ctx, OSyncChange *chg)
{
    // We use the same function for calendar and to-do
    if ( !__access(ctx, chg) )
        return false;
    osync_hashtable_update_hash(hashtable, chg);
    osync_context_report_success(ctx);
    return true;
}
