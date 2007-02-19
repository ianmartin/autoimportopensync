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
//#include "kcal.h"
//#include "knotes.h"
static bool sentinal = false;

class KdePluginImplementation: public KdePluginImplementationBase
{
	public:
		KdePluginImplementation(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
				: mApplication( 0 ),
				mNewApplication( false )
		{}

		void initKDE()
		{
			if (sentinal)
				return;

			KAboutData aboutData(
			    "libopensync-kdepim-plugin",         // internal program name
			    "OpenSync-KDE-plugin",               // displayable program name.
			    "0.2",                               // version string
			    "OpenSync KDEPIM plugin",            // short porgram description
			    KAboutData::License_GPL,             // license type
			    "(c) 2005, Eduardo Pereira Habkost, (c)", // copyright statement
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

		bool init(OSyncPluginInfo *info, OSyncError **error)
		{
			osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);

			initKDE();

			OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
			QString tablepath = QString("%1/hashtable.db").arg(osync_plugin_info_get_configdir(info));
			mHashtable = osync_hashtable_new(tablepath, osync_objtype_sink_get_name(sink), error);

			mKaddrbook = new KContactDataSource(mMember, mHashtable);

			osync_trace(TRACE_EXIT, "%s", __func__);
			return true;
		}

		virtual ~KdePluginImplementation()
		{
			if ( mNewApplication ) {
				delete mApplication;
				mApplication = 0;
			}

			if ( mHashtable )
				osync_hashtable_free(mHashtable);
		}

		virtual void connect(OSyncPluginInfo * /*info*/, OSyncContext *ctx)
		{
			osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);

//			OSyncError *error = NULL;

			if (mKaddrbook && \
			        !mKaddrbook->connect(ctx)) {
				osync_trace(TRACE_EXIT_ERROR, "%s: Unable to open addressbook", __func__);
				return;
			}

			osync_context_report_success(ctx);
			osync_trace(TRACE_EXIT, "%s", __func__);
		}

		virtual void disconnect(OSyncPluginInfo * /*info*/, OSyncContext *ctx)
		{
			if (mKaddrbook && mKaddrbook->connected && !mKaddrbook->disconnect(ctx))
				return;

			osync_context_report_success(ctx);
		}


		virtual void sync_done(OSyncPluginInfo *info, OSyncContext *ctx)
		{
			if (mKaddrbook && mKaddrbook->connected)
			{
				QString anchorpath = QString("%1/anchor.db").arg(osync_plugin_info_get_configdir(info));
				osync_anchor_update(anchorpath, "contact", "true");
			}
			osync_context_report_success(ctx);
		}

		virtual void get_changeinfo(OSyncPluginInfo * /*info*/, OSyncContext *ctx)
		{
			if (mKaddrbook && mKaddrbook->connected && !mKaddrbook->contact_get_changeinfo(ctx))
				return;
			osync_context_report_success(ctx);
		}

		virtual bool vcard_access(OSyncPluginInfo * /*info*/, OSyncContext *ctx, OSyncChange *chg)
		{
			if (mKaddrbook)
				return mKaddrbook->vcard_access(ctx, chg);
			else {
				osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No addressbook loaded");
				return false;
			}
			return true;
		}

		virtual bool vcard_commit_change(OSyncPluginInfo * /*info*/, OSyncContext *ctx, OSyncChange *chg)
		{
			if (mKaddrbook)
				return mKaddrbook->vcard_commit_change(ctx, chg);
			else {
				osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No addressbook loaded");
				return false;
			}
			return true;
		}
	private:
		KContactDataSource *mKaddrbook;

		OSyncHashTable *mHashtable;
		OSyncMember *mMember;

		KApplication *mApplication;
		bool mNewApplication;
};


extern "C"
{

	KdePluginImplementationBase *new_KdePluginImplementation(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error) {
		KdePluginImplementation *imp = new KdePluginImplementation(plugin, info, error);
		if (!imp->init(info, error)) {
			delete imp;
			return 0;
		}

		return imp;
	}

}
