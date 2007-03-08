/***********************************************************************
KAddressbook support for OpenSync kdepim-sync plugin
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
 * @autor Matthias Jahn <jahn.matthias@freenet.de>
 */

#include "kaddrbook.h"
#include <kapplication.h>
#include <dcopclient.h>
#include <qdeepcopy.h>

KContactDataSource::KContactDataSource(OSyncMember *member, OSyncHashTable *hashtable) : converter(new VCardConverter()), hashtable(hashtable), member(member), connected(false)
{
}

KContactDataSource::~KContactDataSource()
{
	delete converter;
	converter = 0;
}

/** Calculate the hash value for an Addressee.
 * Should be called before returning/writing the
 * data, because the revision of the Addressee
 * can be changed.
 */
QString KContactDataSource::calc_hash(Addressee &e)
{
	//Get the revision date of the KDE addressbook entry.
	//Regard entries with invalid revision dates as having just been changed.
	QDateTime revdate = e.revision();
	osync_debug("kde", 3, "Getting hash: %s", revdate.toString().data());
	if (!revdate.isValid()) {
		revdate = QDateTime::currentDateTime();
		e.setRevision(revdate);
	}

	return (revdate.toString());
}

bool KContactDataSource::connect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);

	DCOPClient *dcopc = KApplication::kApplication()->dcopClient();
	if (!dcopc) {
		osync_context_report_error(ctx, OSYNC_ERROR_INITIALIZATION, "Unable to initialize dcop client");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to initialize dcop client", __func__);
		return (false);
	}

	QString appId = dcopc->registerAs("opensync-kaddrbook");

	//get a handle to the standard KDE addressbook
	addressbookptr = StdAddressBook::self(true);
	/*stolen from KPilot*/
        // find out if this can fail for reasons other than a non-existent
        // vcf file. If so, how can I determine if the missing file was the problem
        // or something more serious:
        if ( !addressbookptr || !addressbookptr->load() )
        {
                // Something went wrong, so tell the user and return false to exit the conduit
		osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION,"Unable to initialize and load the addressbook for the sync.");
                return (false);
        }
        abChanged = false;
        ticket=addressbookptr->requestSaveTicket();
        if (!ticket)
        {
		osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION,"Unable to lock addressbook for writing.");
                return (false);
        }

	//Detection mechanismn if this is the first sync
	if (!osync_anchor_compare(member, "contact", "true")) {
		osync_trace(TRACE_INTERNAL, "Setting slow-sync contact");
		osync_member_set_slow_sync(member, "contact", TRUE);
	} 
	else {
		osync_group_reset_slow_sync(osync_member_get_group(member), "contact");
	}

	connected = true;
	osync_trace(TRACE_EXIT, "%s", __func__);
	return (true);
}

bool KContactDataSource::disconnect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);

	if ( !ticket ) {
		osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Unable to get save ticket");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to get save ticket", __func__);
		return (false);
	}
	bool res = false;
	if (abChanged) {
		res = addressbookptr->save( ticket );
	}
	// XXX: KDE4: release ticket in all cases (save no longer releases it)
	if ( !res ) // didn't save, delete ticket manually
	{
		addressbookptr->releaseSaveTicket(ticket);
	}
	ticket=0;
	
	connected = false;
	osync_trace(TRACE_EXIT, "%s", __func__);
	return (true);
}


/** Converts an @c KABC::Addressee object to an @c OSyncChange.
 *
 * @param a The addressee
 *
 * @return An Opensync change with data in vcard format
 */
OSyncChange *KContactDataSource::_addressee_to_change(Addressee *a)
{
	OSyncChange *chg = osync_change_new();

	QString uid = a->uid();

	osync_change_set_member(chg, member);
	osync_change_set_uid(chg, uid.local8Bit());

	QString hash = calc_hash(*a);

	// Convert the VCARD data into a string
	// only vcard3.0 exports Categories
	QString tmp = converter->createVCard(*a, VCardConverter::v3_0);

	char *data = strdup((const char *)tmp.utf8());

	osync_trace(TRACE_SENSITIVE,"\n%s", data);

	osync_change_set_data(chg, data, strlen(data) + 1, TRUE);

	// object type and format
	osync_change_set_objtype_string(chg, "contact");
	osync_change_set_objformat_string(chg, "vcard30");

	osync_change_set_hash(chg, hash.data());

	return (chg);
}

bool KContactDataSource::contact_get_changeinfo(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);

	if (osync_member_get_slow_sync(member, "contact")) {
		osync_trace(TRACE_INTERNAL, "Got slow-sync");
		osync_hashtable_set_slow_sync(hashtable, "contact");
	}

	// We must reload the KDE addressbook in order to retrieve the latest changes.
	if (!addressbookptr->load()) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't reload KDE addressbook");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to reload addrbook", __func__);
		return (false);
	}

	for (AddressBook::Iterator it=addressbookptr->begin(); it!=addressbookptr->end(); it++ ) {
		OSyncChange *chg = NULL;
		chg = _addressee_to_change(&*it);

		// Use the hash table to check if the object
		// needs to be reported
		if (osync_hashtable_detect_change(hashtable, chg)) {
			osync_context_report_change(ctx, chg);
			osync_hashtable_update_hash(hashtable, chg);
		}
	}

	// Use the hashtable to report deletions
	osync_hashtable_report_deleted(hashtable, ctx, "contact");

	osync_trace(TRACE_EXIT, "%s", __func__);
	return (true);
}

/** vcard access method
 *
 * This method is used by both access() and commit_change() method,
 * so it shouldn't call osync_context_report_success(). On success,
 * it should just return true and let the caller report success() to
 * OpenSync
 */
bool KContactDataSource::__vcard_access(OSyncContext *ctx, OSyncChange *chg)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, chg);

	// convert VCARD string from obj->comp into an Addresse object.
	char *data = osync_change_get_data(chg);
	size_t data_size = osync_change_get_datasize(chg);
	QString uid = osync_change_get_uid(chg);

	OSyncChangeType chtype = osync_change_get_changetype(chg);
	switch(chtype) {
		case CHANGE_MODIFIED: {
			Addressee addressee = converter->parseVCard(QString::fromUtf8(data, data_size));

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
		case CHANGE_ADDED: {
			Addressee addressee = converter->parseVCard(QString::fromUtf8(data, data_size));

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
		case CHANGE_DELETED: {
			if (uid.isEmpty()) {
				osync_context_report_error(ctx, OSYNC_ERROR_FILE_NOT_FOUND, "Trying to delete entry with empty UID");
				osync_trace(TRACE_EXIT_ERROR, "%s: Trying to delete but uid is empty", __func__);
				return (false);
			}

			//find addressbook entry with matching UID and delete it
			Addressee addressee = addressbookptr->findByUid(uid);
			if(!addressee.isEmpty())
				addressbookptr->removeAddressee(addressee);

			osync_debug("kde", 3, "KDE ADDRESSBOOK ENTRY DELETED (UID=%s)", (const char*)uid.local8Bit());

			break;
		}
		default: {
			osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Operation not supported");
			osync_trace(TRACE_EXIT_ERROR, "%s: Operation not supported", __func__);
			return (false);
		}
	}
	abChanged = TRUE;
	osync_trace(TRACE_EXIT, "%s", __func__);
	return (true);
}

bool KContactDataSource::vcard_access(OSyncContext *ctx, OSyncChange *chg)
{
	if (!__vcard_access(ctx, chg))
		return (false);

	osync_context_report_success(ctx);
	return (true);
}

bool KContactDataSource::vcard_commit_change(OSyncContext *ctx, OSyncChange *chg)
{
	if ( !__vcard_access(ctx, chg) )
		return (false);

	osync_hashtable_update_hash(hashtable, chg);
	osync_context_report_success(ctx);
	return (true);
}
