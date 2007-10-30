/***********************************************************************
KNotes support for OpenSync kdepim-sync plugin
Copyright (C) 2004 Conectiva S. A.
 
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
/** @file
 *
 * @author Eduardo Pereira Habkost <ehabkost@conectiva.com.br>
 * @author Andrew Baumann <andrewb@cse.unsw.edu.au>
 *
 * This module implements the access to the KDE 3.2 Notes, that are
 * stored on KGlobal::dirs()->saveLocation( "data" , "knotes/" ) + "notes.ics"
 *
 */

#include "knotes.h"
/*An adapted C++ implementation of RSA Data Securities MD5 algorithm.*/
#include <kmdcodec.h>

bool KNotesDataSource::initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, plugin, info);

	if (!OSyncDataSource::initialize(plugin, info, error)) {
		osync_trace(TRACE_EXIT_ERROR, "%s", __PRETTY_FUNCTION__);
		return false;
	}

	osync_objtype_sink_add_objformat(sink, "xmlformat-note");

	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
	return true;
}

void KNotesDataSource::connect(OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __PRETTY_FUNCTION__, info, ctx);
	
	//connect to dcop
	kn_dcop = KApplication::kApplication()->dcopClient();
	if (!kn_dcop) {
		osync_context_report_error(ctx, OSYNC_ERROR_INITIALIZATION, "Unable to make new dcop for knotes");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to make new dcop for knotes", __func__);
		return;
	}

	/*if (!kn_dcop->attach()) {
		osync_context_report_error(ctx, OSYNC_ERROR_INITIALIZATION, "Unable to attach dcop for knotes");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to attach dcop for knotes", __func__);
		return FALSE;
	}*/

	QString appId = kn_dcop->registerAs("opensync");

	//check if kontact is running, and return an error if it
	//is running
	if (kn_dcop->isApplicationRegistered("kontact")) {
		osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, "Kontact is running. Please finish it");
		osync_trace(TRACE_EXIT_ERROR, "%s: Kontact is running", __func__);
		return;
	}

	//check knotes running
	QCStringList apps = kn_dcop->registeredApplications();
	if (!apps.contains("knotes")) {
		//start knotes if not running
		knotesWasRunning = false;
		system("knotes");
		system("dcop knotes KNotesIface hideAllNotes");
	} else
		knotesWasRunning = true;

	kn_iface = new KNotesIface_stub("knotes", "KNotesIface");

	OSyncDataSource::connect(info, ctx);
	
	osync_trace(TRACE_EXIT, "%s", __PRETTY_FUNCTION__);
}

void KNotesDataSource::disconnect(OSyncPluginInfo *, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);

	// FIXME: ugly, but necessary
	if (!knotesWasRunning) {
		system("dcop knotes MainApplication-Interface quit");
	}

	//detach dcop
	/*if (!kn_dcop->detach()) {
		osync_context_report_error(ctx, OSYNC_ERROR_INITIALIZATION, "Unable to detach dcop for knotes");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to detach dcop for knotes", __func__);
		return FALSE;
	}*/
	//destroy dcop
	delete kn_iface;
	kn_iface = NULL;
	//delete kn_dcop;
	//kn_dcop = NULL;

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static QString strip_html(QString input)
{
	osync_trace(TRACE_SENSITIVE, "input is %s\n", (const char*)input.local8Bit());
	QString output = NULL;
	unsigned int i = 0;
	int inbraces = 0;
	for (i = 0; i < input.length(); i++) {
		QCharRef cur = input[i];
		if (cur == '<')
			inbraces = 1;
		if (cur == '>') {
			inbraces = 0;
			continue;
		}
		if (!inbraces)
			output += input[i];
	}
	osync_trace(TRACE_SENSITIVE, "output is %s\n", (const char*)output.stripWhiteSpace().local8Bit());
	return output.stripWhiteSpace();
}

void KNotesDataSource::get_changes(OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	QMap <KNoteID_t,QString> fNotes;
	//set Digest to rawResult
	//KMD5::Digest rawResult;
	KMD5 hash_value;
	OSyncError *error = NULL;

	fNotes = kn_iface->notes();
	if (kn_iface->status() != DCOPStub::CallSucceeded) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to get changed notes");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to get changed notes", __func__);
		return;
	}

	if (osync_objtype_sink_get_slowsync(sink)) {
		osync_trace(TRACE_INTERNAL, "Got slow-sync, resetting hashtable");
		osync_hashtable_reset(hashtable);
	}

	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	OSyncObjFormat *objformat = osync_format_env_find_objformat(formatenv, "xmlformat-note");

	OSyncChange *chg = NULL;
	OSyncData *odata = NULL;
	OSyncChangeType changetype;

	QMap<KNoteID_t,QString>::ConstIterator i;
	for (i = fNotes.begin(); i != fNotes.end(); i++) {
		/* XXX: don't report empty notes, knotes always
		 * "keeps" at least one
		 */
		if (kn_iface->text(i.key()) == "") {
//			osync_debug("knotes", 4, "Skipping empty note");
			continue;
		}

//		osync_debug("knotes", 4, "Note key: %s", (const char*)i.key().local8Bit());
//		osync_debug("knotes", 4, "Note summary: %s", (const char*)i.data().local8Bit());
		osync_trace(TRACE_INTERNAL, "reporting notes %s\n", (const char*)i.key().local8Bit());

		QString uid = i.key();
		QString hash = NULL;

		// Create XMLFormat-note
		OSyncXMLFormat *xmlformat = osync_xmlformat_new("note", &error);
		OSyncXMLField *xmlfield = osync_xmlfield_new(xmlformat, "Summary", &error);

		QCString utf8str = i.data().utf8();
		hash_value.update(utf8str);
		osync_xmlfield_set_key_value(xmlfield, "Content", utf8str);

		utf8str = strip_html(kn_iface->text(i.key())).utf8();
		hash_value.update(utf8str);
		hash = hash_value.base64Digest ();
		if (utf8str && !utf8str.isEmpty()) {
			xmlfield = osync_xmlfield_new(xmlformat, "Body", &error);
			osync_xmlfield_set_key_value(xmlfield, "Content", utf8str);
		}

		// initialize the change object
		chg = osync_change_new(&error);
		if (!chg)
			goto error;
		osync_change_set_uid(chg, uid.local8Bit());

		odata = osync_data_new((char*)xmlformat, sizeof(OSyncXMLFormat *), objformat, &error);
		if (!odata)
			goto error;

		osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));

		//Now you can set the data for the object
		osync_change_set_data(chg, odata);
		osync_data_unref(odata);

//		osync_debug("knotes", 4, "Reporting note:\%s", osync_change_get_printable(chg));

		// Use the hash table to check if the object
		// needs to be reported
		osync_change_set_hash(chg, hash.data());

		// Report entry ... otherwise it gets deleted!
		osync_hashtable_report(hashtable, uid);
		
		changetype = osync_hashtable_get_changetype(hashtable, uid, hash.data());
		osync_change_set_changetype(chg, changetype);
		if (OSYNC_CHANGE_TYPE_UNMODIFIED != changetype) {
			osync_context_report_change(ctx, chg);
			osync_hashtable_update_hash(hashtable, changetype, uid, hash.data());
		}

		hash_value.reset();
	}

	if (!report_deleted(info, ctx, objformat)) {
		osync_trace(TRACE_EXIT_ERROR, "%s", __func__);
		return;
	}

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	if (chg)
		osync_change_unref(chg);
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

void KNotesDataSource::commit(OSyncPluginInfo *, OSyncContext *ctx, OSyncChange *chg)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, chg);
	OSyncChangeType type = osync_change_get_changetype(chg);

	QString uid = osync_change_get_uid(chg);

	//set Digest to rawResult
	//KMD5::Digest rawResult;
	KMD5 hash_value;

	if (type != OSYNC_CHANGE_TYPE_DELETED) {

		// Get data
		OSyncData *odata = osync_change_get_data(chg);
		OSyncXMLFormat *xmlformat = (OSyncXMLFormat *)osync_data_get_data_ptr(odata);

		if (!xmlformat) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to get xmlformat");
			osync_trace(TRACE_EXIT_ERROR, "%s: Invalid data", __func__);
			return;
		}

		if (strcmp("note", osync_xmlformat_get_objtype(xmlformat))) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Wrong xmlformat: %s", osync_xmlformat_get_objtype(xmlformat));
			osync_trace(TRACE_EXIT_ERROR, "%s: Wrong xmlformat.", __func__);
			return;
		}

		QString summary, body;

	       OSyncXMLField *xmlfield = osync_xmlformat_get_first_field(xmlformat);
		for (; xmlfield; xmlfield = osync_xmlfield_get_next(xmlfield)) {
			osync_trace(TRACE_INTERNAL, "Field: %s", osync_xmlfield_get_name(xmlfield));

			if (!strcmp("Body", osync_xmlfield_get_name(xmlfield))) {
				summary = osync_xmlfield_get_key_value(xmlfield, "Content");
			} else if (!strcmp("Summary", osync_xmlfield_get_name(xmlfield))) {
				body = osync_xmlfield_get_key_value(xmlfield, "Content");
			}
		}

		QString hash;
		switch (type) {
			case OSYNC_CHANGE_TYPE_ADDED: {
				osync_trace(TRACE_INTERNAL, "addding new \"%s\" and \"%s\"\n", (const char*)summary.local8Bit(), (const char*)body.local8Bit());
				uid = kn_iface->newNote(summary, body);
				if (kn_iface->status() != DCOPStub::CallSucceeded) {
					osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to add new note");
					osync_trace(TRACE_EXIT_ERROR, "%s: Unable to add new note", __func__);
					return;
				}

				kn_iface->hideNote(uid);
				if (kn_iface->status() != DCOPStub::CallSucceeded)
					osync_trace(TRACE_INTERNAL, "ERROR: Unable to hide note");
				hash_value.update(summary);
				hash_value.update(body);
				hash = hash_value.base64Digest();
				osync_change_set_uid(chg, uid);
				osync_change_set_hash(chg, hash);
				break;
			}
			case OSYNC_CHANGE_TYPE_MODIFIED: {
				kn_iface->setName(uid, summary);
				if (kn_iface->status() != DCOPStub::CallSucceeded) {
					osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to set name");
					osync_trace(TRACE_EXIT_ERROR, "%s: Unable to set name", __func__);
					return;
				}

				kn_iface->setText(uid, body);
				if (kn_iface->status() != DCOPStub::CallSucceeded) {
					osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to set text");
					osync_trace(TRACE_EXIT_ERROR, "%s: Unable to set text", __func__);
					return;
				}
				hash_value.update(summary);
				hash_value.update(body);
				hash = hash_value.base64Digest();
				osync_change_set_hash(chg, hash);
				break;
			}
			default: {
				osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Invalid change type");
				osync_trace(TRACE_EXIT_ERROR, "%s: Invalid change type", __func__);
				return;
			}
		}
	} else {
		system("dcop knotes KNotesIface hideAllNotes");
		QString asdasd = "dcop knotes KNotesIface killNote " + uid + " true";
		system((const char*)asdasd.local8Bit());
//		osync_debug("knotes", 4, "Deleting note %s", (const char*)uid.local8Bit());
		/*kn_iface->killNote(uid, true);
		if (kn_iface->status() != DCOPStub::CallSucceeded) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to delete note");
			osync_trace(TRACE_EXIT_ERROR, "%s: Unable to delete note", __func__);
			return false;
		}*/
	}

	osync_hashtable_update_hash(hashtable, type, uid, osync_change_get_hash(chg));
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
}
