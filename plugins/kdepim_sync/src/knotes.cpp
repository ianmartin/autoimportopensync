/*********************************************************************** 
KNotes support for OpenSync kdepim-sync plugin
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
/** @file
 *
 * @autor Eduardo Pereira Habkost <ehabkost@conectiva.com.br>
 *
 * This module implements the access to the KDE 3.2 Notes, that are
 * stored on KGlobal::dirs()->saveLocation( "data" , "knotes/" ) + "notes.ics"
 *
 * TODO: Check how notes are stored on KDE 3.3/3.4, and use the right interface.
 */

#include <libkcal/calendarlocal.h>
#include <libkcal/icalformat.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kio/netaccess.h>
#include <klocale.h>

#include "knotes.h"

KNotesDataSource::KNotesDataSource(OSyncMember *m, OSyncHashTable *h)
    :member(m), hashtable(h)
{
}

bool KNotesDataSource::connect(OSyncContext *ctx)
{
    // Terminate knotes because we will change its data
    //FIXME: use dcop programming interface, not the commandline utility
#if 1
    if (!system("dcop knotes MainApplication-Interface quit"))
        /* Restart knotes later */
        knotesWasRunning = true;
    else
        knotesWasRunning = false;
#endif

    calendar = new KCal::CalendarLocal;
    if (!calendar) {
        osync_context_report_error(ctx, OSYNC_ERROR_INITIALIZATION, "Couldn't allocate calendar object");
    }
    if (!calendar->load(KGlobal::dirs()->saveLocation( "data" , "knotes/" ) + "notes.ics")) {
        osync_context_report_error(ctx, OSYNC_ERROR_FILE_NOT_FOUND, "Couldn't load notes");
    }

    return true;
}

/** Quick hack to avoid using KIO::NetAccess (that needs a KApplication instance */
static bool copy_file(const char *from, const char *to)
{
    int f1 = open(from, O_RDONLY);
    if (f1 < 0)
        return false;
    int f2 = open(to, O_WRONLY);
    if (f2 < 0) {
        close(f1);
        return false;
    }

    char buf[1024];
    int ret;
    bool result = true;
    for (;;) {
        ret = read(f1, buf, 1024);
        if (ret == 0)
            break;
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            result = false;
            break;
        }
        int wret = write(f2, buf, ret);
        if (wret < ret) {
            result = false;
            break;
        }
    }
    close(f1);
    close(f2);
    return result;
}

bool KNotesDataSource::saveNotes(OSyncContext *ctx)
{
    // shamelessly copied from KNotes source code
    QString file = KGlobal::dirs()->saveLocation( "data" , "knotes/" ) + "notes.ics";
    QString backup = file + "~";

    // if the backup fails don't even try to save the current notes
    // (might just destroy the file that's already there)

    if ( !copy_file(file, backup) )
    {
        osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR,
                            i18n("Unable to save the notes backup to "
                                 "%1! Check that there is sufficient "
                                 "disk space!").arg( backup ) );
        return false;
    }
    else if ( !calendar->save( file, new KCal::ICalFormat() ) )
    {
        osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR,
                            i18n("Unable to save the notes to %1! "
                                 "Check that there is sufficient disk space."
                                 "There should be a backup in %2 "
                                 "though.").arg( file ).arg( backup ) );
        return false;
    }
    return true;
}

bool KNotesDataSource::disconnect(OSyncContext *ctx)
{
    if (!saveNotes(ctx))
        return false;

    delete calendar;
    calendar = NULL;

    // FIXME: ugly, but necessary
#if 1
    if (knotesWasRunning)
        system("knotes");
#endif
    return true;
}

static QString calc_hash(const KCal::Journal *note)
{
    QDateTime d = note->lastModified();
    if (!d.isValid())
        d = QDateTime::currentDateTime();
    /*FIXME: not i18ned string */
    return d.toString();
}

bool KNotesDataSource::get_changeinfo(OSyncContext *ctx)
{
    KCal::Journal::List notes = calendar->journals();

    for (KCal::Journal::List::ConstIterator i = notes.begin(); i != notes.end(); i++) {
        osync_debug("knotes", 4, "Note summary: %s", (const char*)(*i)->summary().local8Bit());
        osync_debug("knotes", 4, "Note contents:\n%s\n====", (const char*)(*i)->description().local8Bit());

        QString uid = (*i)->uid();
        QString hash = calc_hash(*i);

        /*FIXME: temporarily, use this weird-ugly-internal-hardcoded format for the vnote data,
         * to allow testing before OpenSync supports vnote correctly.
         */
        char data[10000];
        snprintf(data, 10000, "BEGIN:VNOTE\n%s\n%s\n",
                                    (const char*)(*i)->summary().local8Bit(),
                                    (const char*)(*i)->description().local8Bit());

        // initialize the change object
        OSyncChange *chg = osync_change_new();
        osync_change_set_uid(chg, uid.local8Bit());
        osync_change_set_member(chg, member);

        // object type and format
        osync_change_set_objtype_string(chg, "note");
        osync_change_set_objformat_string(chg, "vnote");
        osync_change_set_data(chg, strdup(data), strlen(data), 1);

        // Use the hash table to check if the object
        // needs to be reported
        osync_change_set_hash(chg, hash.data());
        if (osync_hashtable_detect_change(hashtable, chg)) {
            osync_context_report_change(ctx, chg);
            osync_hashtable_update_hash(hashtable, chg);
        }
    }

    osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Not implemented yet");
    return false;
}

void KNotesDataSource::get_data(OSyncContext *ctx, OSyncChange *)
{
    osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Not implemented yet");
}

bool KNotesDataSource::__access(OSyncContext *ctx, OSyncChange *chg)
{
    /*FIXME: temporarily, use this weird-ugly-internal-hardcoded format for the vnote data,
     * while vnote support is not completely implemented
     */
    OSyncChangeType type = osync_change_get_changetype(chg);

    QString uid = osync_change_get_uid(chg);

    if (type != CHANGE_DELETED) {
        // begin of the ugly-format parsing
        QString data = QString::fromLocal8Bit(osync_change_get_data(chg), osync_change_get_datasize(chg));
        QStringList lines = QStringList::split("\n", data, true);
        if (lines.size() < 3) {
            osync_context_report_error(ctx, OSYNC_ERROR_CONVERT, "Invalid vnote data");
            return false;
        }
        QString summary = lines[1];
        // remove the first two lines
        lines.remove(lines.begin());
        lines.remove(lines.begin());
        QString desc = lines.join("\n");
        KCal::Journal *j = calendar->journal(uid);

        if (type == CHANGE_MODIFIED && !j)
            type = CHANGE_ADDED;

        QString hash, uid;
        // end of the ugly-format parsing
        switch (type) {
            case CHANGE_ADDED:
                j = new KCal::Journal;
                j->setUid(uid);
                j->setSummary(summary);
                j->setDescription(desc);
                hash = calc_hash(j);
                calendar->addJournal(j);
                uid = j->uid();
                osync_change_set_uid(chg, uid);
                osync_change_set_hash(chg, hash);

            break;
            case CHANGE_MODIFIED:
                j->setSummary(summary);
                j->setDescription(desc);
                hash = calc_hash(j);
                osync_change_set_hash(chg, hash);
            break;
            default:
                osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Invalid change type");
                return false;
        }
    } else {
        KCal::Journal *j = calendar->journal(uid);
        if (j) {
            calendar->deleteJournal(j);
        }
    }

    return true;
}

bool KNotesDataSource::access(OSyncContext *ctx, OSyncChange *chg)
{
    if (!__access(ctx, chg))
        return false;

    osync_context_report_success(ctx);
    return true;
}

bool KNotesDataSource::commit_change(OSyncContext *ctx, OSyncChange *chg)
{
    if (!__access(ctx, chg))
        return false;
    osync_hashtable_update_hash(hashtable, chg);
    osync_context_report_success(ctx);
    return true;
}
