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
#include <libkcal/vcalformat.h>
#include <libkcal/calendarlocal.h>
#include <kdeversion.h>

#include "osyncbase.h"
#include "kcal.h"

char* writeMemVObject(char *s, int *len, VObject *o);

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
    /*FIXME: load and lock here? */
    return true;
}

bool KCalDataSource::disconnect(OSyncContext *)
{
    calendar->save();
    delete calendar;
    calendar = NULL;
    return true;
}

static QString calc_hash(const KCal::Event *e)
{
    QDateTime d = e->lastModified();
    if (!d.isValid())
        d = QDateTime::currentDateTime();
    /*FIXME: not i18ned string */
    return d.toString();
}

bool KCalDataSource::get_changeinfo(OSyncContext *ctx)
{
    KCal::Event::List events = calendar->events();
    osync_debug("kcal", 3, "Number of events: %d", events.size());
    
    for (KCal::Event::List::ConstIterator i = events.begin(); i != events.end(); i++) {
        osync_debug("kcal", 3, "One calendar event");
        KCal::Event *e = *i;
        QString hash = calc_hash(e);

        QString uid = e->uid();

        KCal::VCalFormat format;
        KCal::CalendarLocal cal(calendar->timeZoneId());
        osync_debug("kcal", 3, "timezoneid: %s\n", cal.timeZoneId().latin1());
        cal.addEvent(e->clone());
        /* Ugly workaround to a VCalFormat bug, format.toString()
         * doesn't work, but if save() or load() is called before
         * toString(), the segmentation fault will not happen (as
         * the mCalendar private field will be set)
         */
        format.save(&cal, "");
        QString datastr(format.toString(&cal));
        const char *data = datastr.latin1();

        osync_debug("kcal", 3, "UID: %s\n", uid.latin1());
        OSyncChange *chg = osync_change_new();
        osync_change_set_uid(chg, uid.latin1());
        osync_change_set_member(chg, member);

        // object type and format
        osync_change_set_objtype_string(chg, "calendar");
        osync_change_set_objformat_string(chg, "vcalendar");
        //FIXME: deallocate data somewhere
        osync_change_set_data(chg, strdup(data), strlen(data), 1);

        // Use the hash table to check if the object
        // needs to be reported
        osync_change_set_hash(chg, hash.data());
        if (osync_hashtable_detect_change(hashtable, chg)) {
            osync_context_report_change(ctx, chg);
            osync_hashtable_update_hash(hashtable, chg);
        }
    }
    osync_hashtable_report_deleted(hashtable, ctx, "calendar");
    return true;
}

void KCalDataSource::get_data(OSyncContext *ctx, OSyncChange *)
{
    osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Not implemented yet");
}

bool KCalDataSource::access(OSyncContext *ctx, OSyncChange *)
{
    osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Not implemented yet");
    return false;
}
