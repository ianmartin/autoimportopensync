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
 * @author Eduardo Pereira Habkost <ehabkost@conectiva.com.br>
 * @author Andrew Baumann <andrewb@cse.unsw.edu.au>
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
//#include "knotes.h"

static bool sentinel = false;

class KdePluginImplementation: public KdePluginImplementationBase
{
	private:
		KContactDataSource *kaddrbook;
		KCalSharedResource kcal;
		KCalEventDataSource *kcal_event;
		KCalTodoDataSource *kcal_todo;

		KApplication *application;
		bool newApplication;

		void initKDE()
		{
			if (sentinel)
				return;

			KAboutData aboutData(
			    "libopensync-kdepim-plugin",         // internal program name
			    "OpenSync-KDE-plugin",               // displayable program name.
			    "0.3",                               // version string
			    "OpenSync KDEPIM plugin",            // short porgram description
			    KAboutData::License_GPL,             // license type
			    "(c) 2005, Eduardo Pereira Habkost, (c)", // copyright statement
			    0,                                   // any free form text
			    "http://www.opensync.org",           // program home page address
			    "http://www.opensync.org/newticket"  // bug report email address
			);

			KCmdLineArgs::init( &aboutData );
			if ( kapp ) {
				application = kapp;
				newApplication = false;
			} else {
				application = new KApplication( true, true );
				newApplication = true;
			}

			sentinel = true;
		}

	public:
		KdePluginImplementation() : kcal(), application(NULL), newApplication(false)
		{
			kaddrbook = new KContactDataSource();
			kcal_event = new KCalEventDataSource(&kcal);
			kcal_todo = new KCalTodoDataSource(&kcal);
		}

		bool initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
		{
			osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, plugin, info);

			initKDE();

			if (!kaddrbook->initialize(plugin, info, error))
				goto error;

			if (!kcal_event->initialize(plugin, info, error))
				goto error;

			if (!kcal_todo->initialize(plugin, info, error))
				goto error;

			osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
			return true;

		error:
			osync_trace(TRACE_EXIT_ERROR, "%s: %s", __PRETTY_FUNCTION__, osync_error_print(error));
			return false;
		}

		virtual ~KdePluginImplementation()
		{
			delete kaddrbook;
			delete kcal_event;
			delete kcal_todo;

			if ( newApplication ) {
				delete application;
				application = NULL;
			}
		}
};


extern "C"
{
	KdePluginImplementationBase *new_KdePluginImplementation(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error) {
		KdePluginImplementation *imp = new KdePluginImplementation();
		if (!imp->initialize(plugin, info, error)) {
			delete imp;
			return NULL;
		}

		return imp;
	}
}
