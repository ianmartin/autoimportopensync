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
    private:
        KCalDataSource *kcal;
        KNotesDataSource *knotes;
        KContactDataSource *kaddrbook;
        
        OSyncHashTable *hashtable;
        OSyncMember *member;

        KApplication *application;

			
    public:
        KdePluginImplementation(OSyncMember *memb)
            :member(memb)
        {
        }

		void initKDE() {
			if (sentinal) {
				return;
			}
			
			KAboutData aboutData(
	               "libopensync-kdepim-plugin",                        // internal program name
	               "OpenSync-KDE-plugin",        // displayable program name.
	               "0.1",                           // version string
	               "OpenSync KDEPIM plugin",           // short porgram description
	               KAboutData::License_GPL,         // license type
	               "(c) 2005, Eduardo Pereira Habkost", // copyright statement
	               0,                               // any free form text
	               "http://www.opensync.org",       // program home page address
	               "http://www.opensync.org/newticket"  // bug report email address
	            );
			
            KCmdLineArgs::init(&aboutData);
            application = new KApplication();
            
			sentinal = true;
		}

        bool init(OSyncError **error)
        {
            osync_trace(TRACE_ENTRY, "%s(%p)", __func__, error);

			initKDE();

			hashtable = osync_hashtable_new();
			
            kcal = new KCalDataSource(member, hashtable);
            knotes = new KNotesDataSource(member, hashtable);
            kaddrbook = new KContactDataSource(member, hashtable);

            osync_trace(TRACE_EXIT, "%s", __func__);
            return true;
        }
        
        virtual ~KdePluginImplementation()
        {
            if (kcal) {
                delete kcal;
                kcal = NULL;
            }
            
            if (knotes) {
                delete knotes;
                knotes = NULL;
            }
            
            /*if (application) {
                delete application;
                application = NULL;
            }*/
            
            if (hashtable)
            	osync_hashtable_free(hashtable);
        }

        virtual void connect(OSyncContext *ctx)
        {
        	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
        	
        	OSyncError *error = NULL;
        	if (!osync_hashtable_load(hashtable, member, &error)) {
				osync_context_report_osyncerror(ctx, &error);
				osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
				osync_error_free(&error);
				return;
			}
			
        	
			if (kcal && \
			(osync_member_objtype_enabled(member, "todo") || \
			osync_member_objtype_enabled(member, "event")) && \
			!kcal->connect(ctx)) {
				osync_trace(TRACE_EXIT_ERROR, "%s: Unable to open calendar", __func__);
				return;
			}
			
			if (knotes && \
			osync_member_objtype_enabled(member, "note") && \
			!knotes->connect(ctx)) {
				osync_trace(TRACE_EXIT_ERROR, "%s: Unable to open notes", __func__);
				return;
			}
			
			if (kaddrbook && \
			osync_member_objtype_enabled(member, "contact") && \
			!kaddrbook->connect(ctx)) {
				osync_trace(TRACE_EXIT_ERROR, "%s: Unable to open addressbook", __func__);
				return;
			}
			
			osync_context_report_success(ctx);
            osync_trace(TRACE_EXIT, "%s", __func__);
        }
        
        virtual void disconnect(OSyncContext *ctx)
        {
        	osync_hashtable_close(hashtable);

			if (kcal && kcal->connected && !kcal->disconnect(ctx))
				return;
			if (knotes && knotes->connected && !knotes->disconnect(ctx))
				return;
			if (kaddrbook && kaddrbook->connected && !kaddrbook->disconnect(ctx))
				return;
			
			osync_context_report_success(ctx);
        }

		virtual void get_changeinfo(OSyncContext *ctx)
		{
			if (kaddrbook && kaddrbook->connected && !kaddrbook->contact_get_changeinfo(ctx))
				return;
			
			if (kcal && kcal->connected && !kcal->get_changeinfo_events(ctx))
				return;
			
			if (kcal && kcal->connected && !kcal->get_changeinfo_todos(ctx))
				return;
			
			if (knotes && knotes->connected && !knotes->get_changeinfo(ctx))
				return;
			
			osync_context_report_success(ctx);
		}

        virtual bool vcard_access(OSyncContext *ctx, OSyncChange *chg)
        {
			if (kaddrbook)
				return kaddrbook->vcard_access(ctx, chg);
			else {
				osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No addressbook loaded");
				return false;
			}
			return true;
        }

        virtual bool vcard_commit_change(OSyncContext *ctx, OSyncChange *chg)
        {
			if (kaddrbook)
				return kaddrbook->vcard_commit_change(ctx, chg);
			else {
				osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No addressbook loaded");
				return false;
			}
			return true;
        }

        virtual bool event_access(OSyncContext *ctx, OSyncChange *chg)
        {
            if (kcal)
                return kcal->event_access(ctx, chg);
            else {
                osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No calendar loaded");
                return false;
            }
            return true;
        }

        virtual bool event_commit_change(OSyncContext *ctx, OSyncChange *chg)
        {
            if (kcal)
                return kcal->event_commit_change(ctx, chg);
            else {
                osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No calendar loaded");
                return false;
            }
            return true;
        }

        virtual bool todo_access(OSyncContext *ctx, OSyncChange *chg)
        {
            if (kcal)
                return kcal->todo_access(ctx, chg);
            else {
                osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No calendar loaded");
                return false;
            }
            return true;
        }

        virtual bool todo_commit_change(OSyncContext *ctx, OSyncChange *chg)
        {
            if (kcal)
                return kcal->todo_commit_change(ctx, chg);
            else {
                osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No calendar loaded");
                return false;
            }
            return true;
        }

        virtual bool note_access(OSyncContext *ctx, OSyncChange *chg)
        {
            if (knotes)
                return knotes->access(ctx, chg);
            else {
                osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No notes loaded");
                return false;
            }
            return true;
        }

        virtual bool note_commit_change(OSyncContext *ctx, OSyncChange *chg)
        {
            if (knotes)
                return knotes->commit_change(ctx, chg);
            else {
                osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "No notes loaded");
                return false;
            }
            return true;
        }

};


extern "C" {

KdePluginImplementationBase *new_KdePluginImplementation(OSyncMember *member, OSyncError **error)
{
    KdePluginImplementation *imp = new KdePluginImplementation(member);
    if (!imp->init(error)) {
        delete imp;
        return NULL;
    }

    return imp;
}

}// extern "C"
