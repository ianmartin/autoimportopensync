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

extern "C"
{
#include <opensync/opensync.h>
}

#include <kabc/stdaddressbook.h>
#include <kabc/vcardconverter.h>
#include <kabc/resource.h>
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


/*TODO: check why/if the function below is necessary */
static
void unfold_vcard(char *vcard, size_t *size)
{
    char* in  = vcard;
    char* out = vcard;
    char *end = vcard + *size;
    while ( in < end)
    {
        /* remove any occurrences of "=[CR][LF]"                */
        /* these denote folded line markers in VCARD format.    */
        /* Dont know why, but Evolution uses the leading "="    */
        /* character to (presumably) denote a control sequence. */
        /* This is not quite how I interpret the VCARD RFC2426  */
        /* spec (section 2.6 line delimiting and folding).      */
        /* This seems to work though, so thats the main thing!  */
        if (in[0]=='=' && in[1]==13 && in[2]==10)
            in+=3;
        else
            *out++ = *in++;
    }
    *size = out - vcard;
}

class KdePluginImplementation: public KdePluginImplementationBase
{
    private:
        KABC::AddressBook* addressbookptr;   

        KCalDataSource *kcal;
        KNotesDataSource *knotes;

        OSyncMember *member;
        OSyncHashTable *hashtable;

        KApplication *application;

    public:
        KdePluginImplementation(OSyncMember *memb)
            :member(memb)
        {
        }

        bool init(OSyncError **)
        {
            //osync_debug("kde", 3, "%s(%s)", __FUNCTION__);

            KAboutData aboutData(
                       "opensync-kdepim-plugin",                        // internal program name
                       I18N_NOOP( "OpenSync-KDE-plugin"),        // displayable program name.
                       "0.1",                           // version string
                       I18N_NOOP( "OpenSync KDEPIM plugin" ),           // short porgram description
                       KAboutData::License_GPL,         // license type
                       "(c) 2005, Eduardo Pereira Habkost", // copyright statement
                       0,                               // any free form text
                       "http://www.opensync.org",       // program home page address
                       "http://www.opensync.org/newticket"  // bug report email address
                    );

            KCmdLineArgs::init(&aboutData);
            application = new KApplication();

            //get a handle to the standard KDE addressbook
            addressbookptr = KABC::StdAddressBook::self();

            hashtable = osync_hashtable_new();

            kcal = new KCalDataSource(member, hashtable);
            knotes = new KNotesDataSource(member, hashtable);

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
            if (application) {
                delete application;
                application = NULL;
            }
            if (hashtable)
            	osync_hashtable_free(hashtable);
        }

        virtual bool vcard_access(OSyncContext *ctx, OSyncChange *chg)
        {
            if (__vcard_access(ctx, chg) < 0)
                return false;
            osync_context_report_success(ctx);
            /*FIXME: What should be returned? */
            return true;
        }

        virtual bool vcard_commit_change(OSyncContext *ctx, OSyncChange *chg)
        {
            if ( __vcard_access(ctx, chg) < 0)
                return false;
            osync_hashtable_update_hash(hashtable, chg);
            osync_context_report_success(ctx);
            /*FIXME: What should be returned? */
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
