/***********************************************************************
KNotes OSyncDataSource class
Copyright (C) 2004 Conectiva S. A.
Copyright (C) 2005 Armin Bauer
 
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
 * @autor Armin Bauer <armin.bauer@opensync.org>
 */

#include <kglobal.h>
#include <kstandarddirs.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kapplication.h>
#include <qmap.h>
#include "KNotesIface.h"
#include "KNotesIface_stub.h"
#include <stdio.h>
#include <qtimer.h>
#include <dcopclient.h>
#include <qstring.h>
#include <qstringlist.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef QString KNoteID_t;

extern "C"
{
#include <opensync/opensync.h>
#include <opensync/opensync_xml.h>
}

/** KNotes access implementation interface
 */
class KNotesDataSource
{
	private:
		OSyncMember *member;
		OSyncHashTable *hashtable;

		DCOPClient *kn_dcop;
		KNotesIface_stub *kn_iface;

		/** Ugly hack to restart KNotes if it
		 * was running
		 */
		bool knotesWasRunning;

		/** access() method, used by commit() and access()
		 *
		 * Returns true on succes, but don't send success reporting
		 * to context, because the caller may need to do more
		 * operations
		 */
		bool __access(OSyncContext *ctx, OSyncChange *chg);
		bool saveNotes(OSyncContext *ctx);
	public:
		KNotesDataSource(OSyncMember *member, OSyncHashTable *hashtable);

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

		/** get_changeinfo() method
		 *
		 * On success, returns true, but doesn't call osync_context_report_success()
		 * On error, returns false, after calling osync_context_report_error()
		 */
		bool get_changeinfo(OSyncContext *ctx);

		void get_data(OSyncContext *ctx, OSyncChange *chg);

		/** access() method
		 *
		 * On success, returns true, after calling osync_context_report_success()
		 * On error, returns false, after calling osync_context_report_error()
		 */
		bool access(OSyncContext *ctx, OSyncChange *chg);

		/** commit_change() method
		 *
		 * On success, returns true, after calling osync_context_report_success()
		 * On error, returns false, after calling osync_context_report_error()
		 */
		bool commit_change(OSyncContext *ctx, OSyncChange *chg);
		bool connected;
};
