/*********************************************************************** 
OpenSync Plugin for KDE 3.x
Copyright (C) 2004 Stewart Heitmann <sheitmann@users.sourceforge.net>

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
/*
 * 03 Nov 2004 - Eduardo Pereira Habkost <ehabkost@conectiva.com.br>
 * - Ported to OpenSync plugin interface
 */

extern "C"
{
#include <opensync/opensync.h>
#include "kaddrbook.h"
}

#include <kabc/stdaddressbook.h>
#include <kabc/vcardconverter.h>
#include <kabc/resource.h>
#include <kcmdlineargs.h>
#include <kapplication.h>
#include <klocale.h>
#include <qsignal.h>
#include <qfile.h> 


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

class kaddrbook
{
    private:
        KABC::AddressBook* addressbookptr;   
        KABC::Ticket* addressbookticket;
        QDateTime syncdate, newsyncdate;

        OSyncMember *member;
        OSyncHashTable *hashtable;

    public:        
        kaddrbook(OSyncMember *memb)
            :member(memb)
        {
            //osync_debug("kde", 3, "kdepim_plugin: %s(%s)", __FUNCTION__);

            //get a handle to the standard KDE addressbook
            addressbookptr = KABC::StdAddressBook::self();

            //ensure a NULL Ticket ptr
            addressbookticket=NULL;

            hashtable = osync_hashtable_new();
            osync_hashtable_load(hashtable, member);

        }

        int connect()
        {
            //Lock the addressbook
            addressbookticket = addressbookptr->requestSaveTicket();

            if (!addressbookticket)
            {
                osync_debug("kde", 3, "kdepim_plugin: couldnt lock KDE addressbook, aborting sync.");
                return -1;
            }
            osync_debug("kde", 3, "kdepim_plugin: KDE addressbook locked OK.");

            return 0;
        }

        int disconnect()
        {
            //Unlock the addressbook
            addressbookptr->save(addressbookticket);
            addressbookticket = NULL;

            return 0;
        }


        int get_changes(OSyncContext *ctx)
        {
            //osync_debug("kde", 3, "kdepim_plugin: kaddrbook::%s(newdbs=%d)", __FUNCTION__, newdbs);

            osync_bool slow_sync = osync_member_get_slow_sync(member, "contact");

            //remember when we started this current sync
            newsyncdate = QDateTime::currentDateTime();

            // We must reload the KDE addressbook in order to retrieve the latest changes.
            if (!addressbookptr->load())
            {
                osync_debug("kde", 3, "kdepim_plugin: couldnt reload KDE addressbook, aborting sync.");
                return -1;
            }
            osync_debug("kde", 3, "kdepim_plugin: KDE addressbook reloaded OK.");

            //osync_debug("kde", 3, "%s: %s : plugin UID list has %d entries", __FILE__, __FUNCTION__, uidlist.count());

            //Check the entries of the KDE addressbook against the last entries seen by the sync-engine
            for (KABC::AddressBook::Iterator it=addressbookptr->begin(); it!=addressbookptr->end(); it++ ) {
                //Get the revision date of the KDE addressbook entry.
                //Regard entries with invalid revision dates as having just been changed.
                osync_debug("kde", 3, "new entry, uid: %s", it->uid().latin1());

                QDateTime revdate = it->revision();
                if (!revdate.isValid())
                {
                    revdate = newsyncdate;      //use date of this sync as the revision date.
                    it->setRevision(revdate);   //update the Addressbook entry for future reference.
                }     

                // gmalloc a changed_object for this phonebook entry             
                //FIXME: deallocate it somewhere
                OSyncChange *chg= osync_change_new();
                osync_change_set_member(chg, member);

                QCString hash(revdate.toString());

                osync_change_set_hash(chg, hash);
                osync_change_set_uid(chg, it->uid().latin1());

                // Convert the VCARD data into a string
                KABC::VCardConverter converter;
                QString card = converter.createVCard(*it);
                QString data(card.latin1());
                //FIXME: deallocate data somewhere
                osync_change_set_data(chg, strdup(data), data.length(), 1);

                // set the remaining fields
                osync_change_set_objtype_string(chg, "contact");
                osync_change_set_objformat_string(chg, "vcard");
                osync_change_set_hash(chg, hash.data());
                /*FIXME: slowsync */
                if (osync_hashtable_detect_change(hashtable, chg, slow_sync)) {
                    osync_context_report_change(ctx, chg);
                    osync_hashtable_update_hash(hashtable, chg);
                }
            }

            osync_hashtable_report_deleted(hashtable, ctx, slow_sync);

            return 0;
        }


        int access(OSyncChange *chg)
        {
            //osync_debug("kde", 3, "kdepim_plugin: kaddrbook::%s()",__FUNCTION__);

            int result = 0;
    
            // Ensure we still have a lock on the KDE addressbook (we ought to)
            if (addressbookticket==NULL)
            {
                //This should never happen, but just in case....
                osync_debug("kde", 3, "kdepim_plugin: lock on KDE addressbook was lost, aborting sync.");
                return -1;
            }

            KABC::VCardConverter converter;
    
            OSyncChangeType chtype = osync_change_get_changetype(chg);
            char *uid = osync_change_get_uid(chg);
            /* treat modified objects without UIDs as if they were newly added objects */
            if (chtype == CHANGE_MODIFIED && !uid)
                chtype = CHANGE_ADDED;

            // convert VCARD string from obj->comp into an Addresse object.
            char *data;
            size_t data_size;
            data = (char*)osync_change_get_data(chg);
            data_size = osync_change_get_datasize(chg);

            switch(chtype)
            {
                case CHANGE_MODIFIED:
                {
                    unfold_vcard(data, &data_size);

                    KABC::Addressee addressee = converter.parseVCard(QString::fromLatin1(data, data_size));

                    // ensure it has the correct UID
                    addressee.setUid(QString(uid));

                    // replace the current addressbook entry (if any) with the new one
                    addressbookptr->insertAddressee(addressee);

                    osync_debug("kde", 3, "kdepim_plugin: KDE ADDRESSBOOK ENTRY UPDATED (UID=%s)", uid); 
                    result = 0;
                    break;
                }

                case CHANGE_ADDED:
                {
                    // convert VCARD string from obj->comp into an Addresse object
                    // KABC::VCardConverter doesnt do VCARD unfolding so we must do it ourselves first.
                    unfold_vcard(data, &data_size);
                    KABC::Addressee addressee = converter.parseVCard(QString::fromLatin1(data, data_size));

                    // ensure it has a NULL UID
                    //addressee.setUid(QString(NULL));
                    if (!addressee.uid()) {
                        osync_debug("kde", 1, "New addresse has null uid!");
                        addressee.setUid(KApplication::randomString( 10 ));
                    }


                    // add the new address to the addressbook
                    addressbookptr->insertAddressee(addressee);

                    osync_debug("kde", 3, "kdepim_plugin: KDE ADDRESSBOOK ENTRY ADDED (UID=%s)", addressee.uid().latin1());


                    // return the UID of the new entry along with the result
                    osync_change_set_uid(chg, addressee.uid().latin1());
                    result = 0;
                    break;
                }

                case CHANGE_DELETED:
                {
                    if (uid==NULL)
                    {
                        result = 1;
                        break;
                    }

                    //find addressbook entry with matching UID and delete it
                    KABC::Addressee addressee = addressbookptr->findByUid(QString(uid));
                    if(!addressee.isEmpty())
                       addressbookptr->removeAddressee(addressee);

                    osync_debug("kde", 3, "kdepim_plugin: KDE ADDRESSBOOK ENTRY DELETED (UID=%s)", uid);

                    result = 0;
                    break;
                }
                default:
                    result = 0;
            }
            //Save the changes without dropping the lock
            addressbookticket->resource()->save(addressbookticket);
    
            return result;
        }

        int commit_change(OSyncChange *chg)
        {
            int ret;
            if ( (ret = access(chg)) < 0)
                return ret;
            osync_hashtable_update_hash(hashtable, chg);
            return 0;
        }

};

static KApplication *applicationptr=NULL;
static char name[] = "kde-opensync-plugin";
static char *argv[] = {name,0};

static kaddrbook *addrbook_for_context(OSyncContext *ctx)
{
    return (kaddrbook *)osync_context_get_plugin_data(ctx);
}

static void *kde_initialize(OSyncMember *member)
{
    kaddrbook *addrbook;

    osync_debug("kde", 3, "kdepim_plugin: %s()",__FUNCTION__);

    osync_debug("kde", 3, "kdepim_plugin: %s", __FUNCTION__);
    KCmdLineArgs::init(1, argv, "kde-opensync-plugin", i18n("KOpenSync"), "KDE OpenSync plugin", "0.1", false);
    applicationptr = new KApplication();

    /* Allocate and initialise a kaddrbook object. */
    addrbook = new kaddrbook(member);
    if (!addrbook)
        //FIXME: Check if OSYNC_ERROR_GENERIC is the righ error code in this case
        return NULL;

    /* Return kaddrbook object to the sync engine */
    return (void*)addrbook;
}

static void kde_finalize(void *data)
{
    osync_debug("kde", 3, "kdepim_plugin: %s()", __FUNCTION__);

    kaddrbook *addrbook = (kaddrbook *)data;
    delete addrbook;

    if (applicationptr) {
        delete applicationptr;
        applicationptr = 0;
    }
}

static void kde_connect(OSyncContext *ctx)
{
    kaddrbook *addrbook = addrbook_for_context(ctx);
    if (addrbook->connect() < 0) {
        osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't lock KDE addressbook");
        return;
    }
    osync_context_report_success(ctx);
}


static void kde_disconnect(OSyncContext *ctx)
{
    kaddrbook *addrbook = addrbook_for_context(ctx);
    if (addrbook->disconnect() < 0) {
        osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't lock KDE addressbook");
        return;
    }
    osync_context_report_success(ctx);
}

static void kde_get_changeinfo(OSyncContext *ctx)
{
    kaddrbook *addrbook = addrbook_for_context(ctx);
    osync_debug("kde", 3, "kdepim_plugin: %s",__FUNCTION__);

    int err = addrbook->get_changes(ctx);
    if (err) {
        osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't access KDE addressbook");
        return;
    }
    osync_context_report_success(ctx);
    return;
}


static osync_bool kde_commit_change(OSyncContext *ctx, OSyncChange *change)
{
    kaddrbook *addrbook = addrbook_for_context(ctx);
    int err;

    osync_debug("kde", 3, "kdepim_plugin: %s()",__FUNCTION__);

    err = addrbook->commit_change(change); 

    if (err)
        osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't update KDE addressbook");
    else 
        osync_context_report_success(ctx);

    /*FIXME: What should be returned? */
    return true;
}

static osync_bool kde_access(OSyncContext *ctx, OSyncChange *change)
{
    kaddrbook *addrbook = addrbook_for_context(ctx);
    int err;

    osync_debug("kde", 3, "kdepim_plugin: %s()",__FUNCTION__);

    err = addrbook->access(change); 

    if (err)
        osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't update KDE addressbook");
    else 
        osync_context_report_success(ctx);

    /*FIXME: What should be returned? */
    return true;
}

extern "C" {
void get_info(OSyncPluginInfo *info)
{
    info->version = 1;
    info->name = "kde-sync";
    info->description = i18n("Plugin for the KDE 3.x Addressbook");

    info->functions.initialize = kde_initialize;
    info->functions.connect = kde_connect;
    info->functions.disconnect = kde_disconnect;
    info->functions.finalize = kde_finalize;
    info->functions.get_changeinfo = kde_get_changeinfo;

    osync_plugin_accept_objtype(info, "contact");
    osync_plugin_accept_objformat(info, "contact", "vcard");
    osync_plugin_set_commit_objformat(info, "contact", "vcard", kde_commit_change);
    osync_plugin_set_access_objformat(info, "contact", "vcard", kde_access);

}

}// extern "C"
