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

        /** Calculate the hash value for an Addressee.
         * Should be called before returning/writing the
         * data, because the revision of the Addressee
         * can be changed.
         */
        QString calc_hash(KABC::Addressee &e)
        {
            //Get the revision date of the KDE addressbook entry.
            //Regard entries with invalid revision dates as having just been changed.
            QDateTime revdate = e.revision();
            osync_debug("kde", 3, "Getting hash: %s", revdate.toString().data());
            if (!revdate.isValid())
            {
            	revdate = QDateTime::currentDateTime();
            	e.setRevision(revdate);
            }

            return revdate.toString();
        }

        virtual void connect(OSyncContext *ctx)
        {
            //Lock the addressbook
            //addressbookticket = addressbookptr->requestSaveTicket();

			OSyncError *error = NULL;
			if (!osync_hashtable_load(hashtable, member, &error))
			{
				osync_context_report_osyncerror(ctx, &error);
				return;
			}

			//Detection mechanismn if this is the first sync
			if (!osync_anchor_compare(member, "synced", "true")) {
				osync_member_set_slow_sync(member, "contact", TRUE);
				osync_anchor_update(member, "synced", "true");
			}

            /*if (!addressbookticket)
            {
                osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't lock KDE addressbook");
                return;
            }*/
            osync_debug("kde", 3, "KDE addressbook locked OK.");

            if (kcal && !kcal->connect(ctx))
                return;
            if (knotes && !knotes->connect(ctx))
                return;
            osync_context_report_success(ctx);
        }

        virtual void disconnect(OSyncContext *ctx)
        {
            //Unlock the addressbook
            //addressbookptr->save(addressbookticket);
            //addressbookticket = NULL;

			KABC::Ticket *ticket = addressbookptr->requestSaveTicket();
		    if ( !ticket ) {
		      osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Unable to get save ticket");
		      return;
		    }
		
		    if ( !addressbookptr->save( ticket ) ) {
		      osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Unable to get save using ticket");
		      return;
		    }

			osync_hashtable_close(hashtable);

            if (kcal && !kcal->disconnect(ctx))
                return;
            if (knotes && !knotes->disconnect(ctx))
                return;
            osync_context_report_success(ctx);
        }


        /*FIXME: move kaddrbook implementation to kaddrbook.cpp */
        bool addrbook_get_changeinfo(OSyncContext *ctx)
        {
            //osync_debug("kde", 3, "kaddrbook::%s(newdbs=%d)", __FUNCTION__, newdbs);

            if (osync_member_get_slow_sync(member, "contact"))
                osync_hashtable_set_slow_sync(hashtable, "contact");

            // We must reload the KDE addressbook in order to retrieve the latest changes.
            if (!addressbookptr->load()) {
                osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't reload KDE addressbook");
                return false;
            }
            osync_debug("kde", 3, "KDE addressbook reloaded OK.");

            KABC::VCardConverter converter;

            for (KABC::AddressBook::Iterator it=addressbookptr->begin(); it!=addressbookptr->end(); it++ ) {
                QString uid = it->uid();

                OSyncChange *chg = osync_change_new();
                
                osync_change_set_member(chg, member);
                osync_change_set_uid(chg, uid.local8Bit());

				QString hash = calc_hash(*it);
				
                // Convert the VCARD data into a string
                const char *data = converter.createVCard(*it).utf8();
                osync_change_set_data(chg, strdup(data), strlen(data) + 1, TRUE);

                // object type and format
                osync_change_set_objtype_string(chg, "contact");
                osync_change_set_objformat_string(chg, "vcard21");

                // Use the hash table to check if the object
                // needs to be reported
                osync_change_set_hash(chg, hash.data());
                if (osync_hashtable_detect_change(hashtable, chg)) {
                    osync_context_report_change(ctx, chg);
                    osync_hashtable_update_hash(hashtable, chg);
                }
            }

            // Use the hashtable to report deletions
            osync_hashtable_report_deleted(hashtable, ctx, "contact");

            return true;
        }

        virtual void get_changeinfo(OSyncContext *ctx)
        {
            if (!addrbook_get_changeinfo(ctx))
                return;
            if (kcal && !kcal->get_changeinfo_events(ctx))
                return;

            if (kcal && !kcal->get_changeinfo_todos(ctx))
                return;

            if (knotes && !knotes->get_changeinfo(ctx))
                return;

            osync_context_report_success(ctx);
        }

        void kabc_get_data(OSyncContext *ctx, OSyncChange *chg)
        {
            QString uid = osync_change_get_uid(chg);
            KABC::Addressee a = addressbookptr->findByUid(uid);
            KABC::VCardConverter converter;
            QCString card = converter.createVCard(a).utf8();
            const char *data = card;
            //FIXME: deallocate data somewhere
            osync_change_set_data(chg, strdup(data), strlen(data), 1);
            osync_context_report_success(ctx);
        }

        
        virtual void get_data(OSyncContext *ctx, OSyncChange *)
        {
            /*
            switch (osync_change_get_objtype(chg)) {
                case contact:
                kabc_get_data(ctx, chg);
                break;
                case calendar:
                kcal->get_data(ctx, chg);
                break;
                default:
                osync_context_report_error(ctx, OSYNC_ERROR_FILE_NOT_FOUND, "Invalid UID");
                return;
            }
            osync_context_report_success(ctx);
            */
            osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Not implemented yet");
        }
        


        /** Access an object, without returning success
         *
         * returns 0 on success, < 0 on error.
         * If an error occurss, the error will be already reported
         * using osync_context_report_error()
         */
        int __vcard_access(OSyncContext *ctx, OSyncChange *chg)
        {
            KABC::VCardConverter converter;
    
            // convert VCARD string from obj->comp into an Addresse object.
            char *data = osync_change_get_data(chg);
            size_t data_size = osync_change_get_datasize(chg);
			QString uid = osync_change_get_uid(chg);
			
			OSyncChangeType chtype = osync_change_get_changetype(chg);
            switch(chtype)
            {
                case CHANGE_MODIFIED:
                {
                    unfold_vcard(data, &data_size);
                    KABC::Addressee addressee = converter.parseVCard(QString::fromUtf8(data, data_size));

                    // ensure it has the correct UID and revision
                    addressee.setUid(uid);
                    addressee.setRevision(QDateTime::currentDateTime());

                    // replace the current addressbook entry (if any) with the new one
					
                    addressbookptr->insertAddressee(addressee);
                    
                    QString hash = calc_hash(addressee);
                    osync_change_set_hash(chg, hash);
                    osync_debug("kde", 3, "KDE ADDRESSBOOK ENTRY UPDATED (UID=%s)", (const char *)uid.local8Bit()); 
                    break;
                }

                case CHANGE_ADDED:
                {
                    unfold_vcard(data, &data_size);
                    KABC::Addressee addressee = converter.parseVCard(QString::fromUtf8(data, data_size));

                   	// ensure it has the correct revision
                    addressee.setRevision(QDateTime::currentDateTime());

                    // add the new address to the addressbook
                    addressbookptr->insertAddressee(addressee);

                    osync_change_set_uid(chg, addressee.uid().local8Bit());
                    
                    QString hash = calc_hash(addressee);
                    osync_change_set_hash(chg, hash);
                    osync_debug("kde", 3, "KDE ADDRESSBOOK ENTRY ADDED (UID=%s)", (const char *)addressee.uid().local8Bit());

                    break;
                }

                case CHANGE_DELETED:
                {
                    if (uid.isEmpty())
                    {
                        osync_context_report_error(ctx, OSYNC_ERROR_FILE_NOT_FOUND, "Trying to delete entry with empty UID");
                        return -1;
                    }

                    //find addressbook entry with matching UID and delete it
                    KABC::Addressee addressee = addressbookptr->findByUid(uid);
                    if(!addressee.isEmpty())
                       addressbookptr->removeAddressee(addressee);

                    osync_debug("kde", 3, "KDE ADDRESSBOOK ENTRY DELETED (UID=%s)", (const char*)uid.local8Bit());

                    break;
                }
                default:
                    osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Operation not supported");
                    return -1;
            }
    
            return 0;
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
