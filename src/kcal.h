/*********************************************************************** 
KCalendar OSyncDataSource class
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

#include "osyncbase.h"

/*FIXME: maybe create a parent abstract class for all
 * KDE Data sources/sinks: OSyncDataSourceBase
 */
class KCalDataSource
{
    private:
        KCal::CalendarResources *calendar;
        KCal::CalendarResources::Ticket *saveTicket;

        OSyncHashTable *hashtable;
        OSyncMember *member;

        /** access() method, used by commit() and access()
         *
         * Returns true on succes, but don't send success reporting
         * to context, because the caller may need to do more
         * operations
         */
        bool __access(OSyncContext *ctx, OSyncChange *chg);
    public:
        KCalDataSource(OSyncMember *member, OSyncHashTable *hashtable);

        /** connect() method
         *
         * On success, returns true, but doesn't call osync_context_report_success()
         * On error, returns false, after calling osync_context_report_error()
         */
        bool connect(OSyncContext *ctx);

        /** disconnect() method
         *
         * On success, returns true, but doesn't call osync_context_report_success()
         * On error, returns false, after calling osync_context_report_error()
         */
        bool disconnect(OSyncContext *ctx);

        /** get_changeinfo() method.
         *
         * On success, returns true, but doesn't call osync_context_report_success()
         * On error, returns false, after calling osync_context_report_error()
         */
        bool get_changeinfo(OSyncContext *ctx);

        void get_data(OSyncContext *ctx, OSyncChange *chg);

        /** access() method.
         *
         * On success, returns true, after calling osync_context_report_success()
         * On error, returns false, after calling osync_context_report_error()
         */
        bool access(OSyncContext *ctx, OSyncChange *chg);

        /** commit_change() method.
         *
         * On success, returns true, after calling osync_context_report_success()
         * On error, returns false, after calling osync_context_report_error()
         */
         bool commit_change(OSyncContext *ctx, OSyncChange *chg);

};
