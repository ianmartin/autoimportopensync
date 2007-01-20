/***********************************************************************
Actual implementation of the KDE PIM OpenSync plugin
Copyright (C) 2004 Conectiva S. A.
Based on code Copyright (C) 2004 Stewart Heitmann <sheitmann@users.sourceforge.net>
 
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




#include <libkcal/resourcecalendar.h>
#include <kinstance.h>
#include <klocale.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>

#include <qsignal.h>


#include <qfile.h>
#include <dlfcn.h>

#include "osyncbase.h"
#include "kaddrbook.h"
#include "kcal.h"
#include "knotes.h"
static bool sentinal = false;

class KdePluginImplementation: public KdePluginImplementationBase
{
	public:
		KdePluginImplementation( OSyncMember *member )
				: mMember( member ),
				mApplication( 0 ),
				mNewApplication( false )
		{}

		void initKDE()
		{
			if (sentinal)
				return;

			KAboutData aboutData(
			    "libopensync-kdepim-plugin",         // internal program name
			    "OpenSync-KDE-plugin",               // displayable program name.
			    "0.1",                               // version string
			    "OpenSync KDEPIM plugin",            // short porgram description
			    KAboutData::License_GPL,             // license type
			    "(c) 2005, Eduardo Pereira Habkost", // copyright statement
			    0,                                   // any free form text
			    "http://www.opensync.org",           // program home page address
			    "http://www.opensync.org/newticket"  // bug report email address
			);

			KCmdLineArgs::init( &aboutData );
			if ( kapp )
				mApplication = kapp;
			else {
				mApplication = new KApplication( true, true );
				mNewApplication = true;
			}

			sentinal = true;
		}

		bool init(OSyncError **error)
		{
			osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);

			initKDE();

			mHashtable = osync_hashtable_new();

			mKcal = new KCalDataSource(mMember, mHashtable);
			mKnotes = new KNotesDataSource(mMember, mHashtable);
			mKaddrbook = new KContactDataSource(mMember, mHashtable);

			osync_trace(TRACE_EXIT, "%s", __func__);
			return true;
		}

		virtual ~KdePluginImplementation()
		{
			delete mKcal;
			mKcal = 0;

			delete mKnotes;
			mKnotes = 0;

			if ( mNewApplication ) {
				delete mApplication;
				mApplication = 0;
			}

			if ( mHashtable )
				osync_hashtable_free(mHashtable);
		}

		virtual void connect(OSyncContext *ctx)
		{
			osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);

			OSyncError *error = NULL;
			if ( !osync_hashtable_load(mHashtable, mMember, &error) ) {
				osync_context_report_osyncerror(ctx, &error);
				osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
				osync_error_free(&error);
				return;
			}


			if (mKcal && (osync_member_objtype_enabled(mMember, "todo") ||
			              osync_member_objtype_enabled(mMember, "event")) && !mKcal->connect(ctx)) {
				osync_trace(TRACE_EXIT_ERROR, "%s: Unable to open calendar", __func__);
				return;
			}

			if (mKnotes && osync_member_objtype_enabled(mMember, "note") && \
			        !mKnotes->connect(ctx)) {
				osync_trace(TRACE_EXIT_ERROR, "%s: Unable to open notes", __func__);
				return;
			}

			if (mKaddrbook && osync_member_objtype_enabled(mMember, "contact") && \
			        !mKaddrbook->connect(ctx)) {
				osync_trace(TRACE_EXIT_ERROR, "%s: Unable to open addressbook", __func__);
				return;
			}

			osync_context_report_success(ctx);
			osync_trace(TRACE_EXIT, "%s", __func__);
		}

		virtual void disconnect(OSyncContext *ctx)
		{
			osync_hashtable_close(mHashtable);

			if (mKcal && mKcal->connected && !mKcal->disconnect(ctx))
				return;
			if (mKnotes && mKnotes->connected && !mKnotes->disconnect(ctx))
				return;
			if (mKaddrbook && mKaddrbook->connected && !mKaddrbook->disconnect(ctx))
				return;

			osync_context_report_success(ctx);
		}

		virtual void get_changeinfo(OSyncContext *ctx)
		{
			if (mKaddrbook && mKaddrbook->connected && !mKaddrbook->contact_get_changeinfo(ctx))
				return;

			if (mKcal && mKcal->connected && !mKcal->get_changeinfo_events(ctx))
				return;

			if (mKcal && mKcal->connected && !mKcal->get_changeinfo_todos(ctx))
				return;

			if (mKnotes && mKnotes->connected && !mKnotes->get_changeinfo(ctx))
				return;

			osync_context_report_success(ctx);
		}

		virtual bool vcard_access(OSyncContext *ctx, OSyncChange *chg)
		{
			if (mKaddrbook)
				return mKaddrbook->vcard_access(ctx, chg);
			else {
				osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No addressbook loaded");
				return false;
			}
			return true;
		}

		virtual bool vcard_commit_change(OSyncContext *ctx, OSyncChange *chg)
		{
			if (mKaddrbook)
				return mKaddrbook->vcard_commit_change(ctx, chg);
			else {
				osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No addressbook loaded");
				return false;
			}
			return true;
		}

		virtual bool event_access(OSyncContext *ctx, OSyncChange *chg)
		{
			if (mKcal)
				return mKcal->event_access(ctx, chg);
			else {
				osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No calendar loaded");
				return false;
			}
			return true;
		}

		virtual bool event_commit_change(OSyncContext *ctx, OSyncChange *chg)
		{
			if (mKcal)
				return mKcal->event_commit_change(ctx, chg);
			else {
				osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No calendar loaded");
				return false;
			}
			return true;
		}

		virtual bool todo_access(OSyncContext *ctx, OSyncChange *chg)
		{
			if (mKcal)
				return mKcal->todo_access(ctx, chg);
			else {
				osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No calendar loaded");
				return false;
			}
			return true;
		}

		virtual bool todo_commit_change(OSyncContext *ctx, OSyncChange *chg)
		{
			if (mKcal)
				return mKcal->todo_commit_change(ctx, chg);
			else {
				osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No calendar loaded");
				return false;
			}
			return true;
		}

		virtual bool note_access(OSyncContext *ctx, OSyncChange *chg)
		{
			if (mKnotes)
				return mKnotes->access(ctx, chg);
			else {
				osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No notes loaded");
				return false;
			}
			return true;
		}

		virtual bool note_commit_change(OSyncContext *ctx, OSyncChange *chg)
		{
			if (mKnotes)
				return mKnotes->commit_change(ctx, chg);
			else {
				osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No notes loaded");
				return false;
			}
			return true;
		}

	private:
		KCalDataSource *mKcal;
		KNotesDataSource *mKnotes;
		KContactDataSource *mKaddrbook;

		OSyncHashTable *mHashtable;
		OSyncMember *mMember;

		KApplication *mApplication;
		bool mNewApplication;
};


extern "C"
{

	KdePluginImplementationBase *new_KdePluginImplementation(OSyncMember *member, OSyncError **error) {
		KdePluginImplementation *imp = new KdePluginImplementation(member);
		if (!imp->init(error)) {
			delete imp;
			return 0;
		}

		return imp;
	}

}
