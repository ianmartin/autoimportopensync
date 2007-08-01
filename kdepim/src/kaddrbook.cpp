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
 * @author Eduardo Pereira Habkost <ehabkost@conectiva.com.br>
 * @author Armin Bauer <armin.bauer@opensync.org>
 * @author Andrew Baumann <andrewb@cse.unsw.edu.au>
 */

#include "kaddrbook.h"
#include <kapplication.h>
#include <dcopclient.h>
#include <qdeepcopy.h>

bool KContactDataSource::initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, plugin, info);

	if (!OSyncDataSource::initialize(plugin, info, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s", __PRETTY_FUNCTION__);
		return false;
	}

	osync_objtype_sink_add_objformat(sink, "vcard30");

	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
	return true;
}

/** Calculate the hash value for an Addressee.
 * Should be called before returning/writing the
 * data, because the revision of the Addressee
 * can be changed.
 */
QString KContactDataSource::calc_hash(KABC::Addressee &e)
{
	//Get the revision date of the KDE addressbook entry.
	//Regard entries with invalid revision dates as having just been changed.
	QDateTime revdate = e.revision();
//	osync_debug("kde", 3, "Getting hash: %s", revdate.toString().data());
	if (!revdate.isValid()) {
		revdate = QDateTime::currentDateTime();
		e.setRevision(revdate);
	}

	return revdate.toString();
}

void KContactDataSource::connect(OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, info, ctx);

	DCOPClient *dcopc = KApplication::kApplication()->dcopClient();
	if (!dcopc) {
		osync_context_report_error(ctx, OSYNC_ERROR_INITIALIZATION, "Unable to initialize dcop client");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to initialize dcop client", __PRETTY_FUNCTION__);
		return;
	}

	QString appId = dcopc->registerAs("opensync-kaddrbook");

	//check if kaddressbook is running, and return an error if it
	//is running
	if (dcopc->isApplicationRegistered("kaddressbook")) {
		osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, "KAddressBook is running. Please terminate it");
		osync_trace(TRACE_EXIT_ERROR, "%s: KAddressBook is running", __PRETTY_FUNCTION__);
		return;
	}

	//get a handle to the standard KDE addressbook
	addressbookptr = KABC::StdAddressBook::self();
	
	OSyncDataSource::connect(info, ctx);
	
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

void KContactDataSource::disconnect(OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, info, ctx);

	KABC::Ticket *ticket = addressbookptr->requestSaveTicket();
	if ( !ticket ) {
		osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Unable to get save ticket");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to get save ticket", __PRETTY_FUNCTION__);
		return;
	}

	if ( !addressbookptr->save( ticket ) ) {
		osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Unable to use ticket");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to save", __PRETTY_FUNCTION__);
		return;
	}

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
	return;
}

void KContactDataSource::get_changes(OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, info, ctx);

	if (osync_objtype_sink_get_slowsync(sink)) {
		osync_trace(TRACE_INTERNAL, "Got slow-sync, resetting hashtable");
		osync_hashtable_reset(hashtable);
	}

	// We must reload the KDE addressbook in order to retrieve the latest changes.
	if (!addressbookptr->load()) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Couldn't reload KDE addressbook");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to reload addrbook", __PRETTY_FUNCTION__);
		return;
	}

	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	OSyncObjFormat *objformat = osync_format_env_find_objformat(formatenv, "vcard30");

	KABC::VCardConverter converter;
	for (KABC::AddressBook::Iterator it=addressbookptr->begin(); it!=addressbookptr->end(); it++ ) {
		// Convert the VCARD data into a string
		// only vcard3.0 exports Categories
		QString data = converter.createVCard(*it, KABC::VCardConverter::v3_0);

		if (!report_change(info, ctx, it->uid(), data, calc_hash(*it), objformat)) {
			osync_trace(TRACE_EXIT_ERROR, "%s", __PRETTY_FUNCTION__);
			return;
		}
	}

	if (!report_deleted(info, ctx)) {
		osync_trace(TRACE_EXIT_ERROR, "%s", __PRETTY_FUNCTION__);
		return;
	}

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

void KContactDataSource::commit(OSyncPluginInfo *, OSyncContext *ctx, OSyncChange *chg)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, ctx, chg);
	KABC::VCardConverter converter;

	// convert VCARD string from obj->comp into an Addresse object.
	OSyncData *odata = osync_change_get_data(chg);

	char *data;
	//size_t data_size;
	// osync_data_get_data requires an unsigned int which is not compatible with size_t on 64bit machines
	unsigned int data_size = 0;

	osync_data_get_data(odata, &data, &data_size);

	QString uid = osync_change_get_uid(chg);

	OSyncChangeType chtype = osync_change_get_changetype(chg);
	switch(chtype) {
		case OSYNC_CHANGE_TYPE_MODIFIED: {
			KABC::Addressee addressee = converter.parseVCard(QString::fromUtf8(data, data_size));

			// ensure it has the correct UID and revision
			addressee.setUid(uid);
			addressee.setRevision(QDateTime::currentDateTime());

			// replace the current addressbook entry (if any) with the new one

			addressbookptr->insertAddressee(addressee);

			QString hash = calc_hash(addressee);
			osync_change_set_hash(chg, hash);
//			osync_debug("kde", 3, "KDE ADDRESSBOOK ENTRY UPDATED (UID=%s)", (const char *)uid.local8Bit());
			break;
		}
		case OSYNC_CHANGE_TYPE_ADDED: {
			KABC::Addressee addressee = converter.parseVCard(QString::fromUtf8(data, data_size));

			// ensure it has the correct revision
			addressee.setRevision(QDateTime::currentDateTime());

			// add the new address to the addressbook
			addressbookptr->insertAddressee(addressee);

			osync_change_set_uid(chg, addressee.uid().local8Bit());

			QString hash = calc_hash(addressee);
			osync_change_set_hash(chg, hash);
//			osync_debug("kde", 3, "KDE ADDRESSBOOK ENTRY ADDED (UID=%s)", (const char *)addressee.uid().local8Bit());
			break;
		}
		case OSYNC_CHANGE_TYPE_DELETED: {
			if (uid.isEmpty()) {
				osync_context_report_error(ctx, OSYNC_ERROR_FILE_NOT_FOUND, "Trying to delete entry with empty UID");
				osync_trace(TRACE_EXIT_ERROR, "%s: Trying to delete but uid is empty", __PRETTY_FUNCTION__);
				return;
			}

			//find addressbook entry with matching UID and delete it
			KABC::Addressee addressee = addressbookptr->findByUid(uid);
			if(!addressee.isEmpty())
				addressbookptr->removeAddressee(addressee);

//			osync_debug("kde", 3, "KDE ADDRESSBOOK ENTRY DELETED (UID=%s)", (const char*)uid.local8Bit());

			break;
		}
		default: {
			osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Operation not supported");
			osync_trace(TRACE_EXIT_ERROR, "%s: Operation not supported", __PRETTY_FUNCTION__);
			return;
		}
	}

	osync_hashtable_update_hash(hashtable, chtype, uid, osync_change_get_hash(chg));

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}
