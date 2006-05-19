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
 * @autor Eduardo Pereira Habkost <ehabkost@conectiva.com.br>
 *
 * This module implements the access to the KDE 3.2 Notes, that are
 * stored on KGlobal::dirs()->saveLocation( "data" , "knotes/" ) + "notes.ics"
 *
 */



#include "knotes.h"


KNotesDataSource::KNotesDataSource(OSyncMember *m, OSyncHashTable *h)
    :member(m), hashtable(h)
{
	connected = false;
}

bool KNotesDataSource::connect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	
	//connect to dcop
	kn_dcop = KApplication::kApplication()->dcopClient();
	if (!kn_dcop) {
		osync_context_report_error(ctx, OSYNC_ERROR_INITIALIZATION, "Unable to make new dcop for knotes");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to make new dcop for knotes", __func__);
		return FALSE;
	}
	
	/*if (!kn_dcop->attach()) {
		osync_context_report_error(ctx, OSYNC_ERROR_INITIALIZATION, "Unable to attach dcop for knotes");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to attach dcop for knotes", __func__);
		return FALSE;
	}*/
	
	QString appId = kn_dcop->registerAs("opensync");
	
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

	connected = true;
	osync_trace(TRACE_EXIT, "%s", __func__);
	return true;
}

bool KNotesDataSource::disconnect(OSyncContext *ctx)
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
	
	connected = false;
	osync_trace(TRACE_EXIT, "%s", __func__);
	return true;
}

static QString strip_html(QString input)
{
	osync_trace(TRACE_INTERNAL, "input is %s\n", (const char*)input.local8Bit());
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
	osync_trace(TRACE_INTERNAL, "output is %s\n", (const char*)output.stripWhiteSpace().local8Bit());
	return output.stripWhiteSpace();
}

bool KNotesDataSource::get_changeinfo(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	QMap <KNoteID_t,QString> fNotes;
	
	fNotes = kn_iface->notes();
	if (kn_iface->status() != DCOPStub::CallSucceeded) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to get changed notes");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to get changed notes", __func__);
		return FALSE;
	}
	
	if (osync_member_get_slow_sync(member, "note")) {
		osync_debug("kcal", 3, "Setting slow-sync for notes");
		osync_hashtable_set_slow_sync(hashtable, "note");
	}
	
	QMap<KNoteID_t,QString>::ConstIterator i;
	for (i = fNotes.begin(); i != fNotes.end(); i++) {
		osync_debug("knotes", 4, "Note key: %s", (const char*)i.key().local8Bit());
        osync_debug("knotes", 4, "Note summary: %s", (const char*)i.data().local8Bit());
		osync_trace(TRACE_INTERNAL, "reporting notes %s\n", (const char*)i.key().local8Bit());
		
        QString uid = i.key();
		QString hash = NULL;
        // Create osxml doc containing the note
        xmlDoc *doc = xmlNewDoc((const xmlChar*)"1.0");
        xmlNode *root = osxml_node_add_root(doc, "Note");

        OSyncXMLEncoding enc;
        enc.encoding = OSXML_8BIT;
        enc.charset = OSXML_UTF8;

        // Set the right attributes
        xmlNode *sum = xmlNewChild(root, NULL, (const xmlChar*)"", NULL);
        QCString utf8str = i.data().utf8();
        hash = utf8str;
        osxml_node_set(sum, "Summary", utf8str, enc);

        xmlNode *body = xmlNewChild(root, NULL, (const xmlChar*)"", NULL);
        utf8str = strip_html(kn_iface->text(i.key())).utf8();
        hash += utf8str;
        osxml_node_set(body, "Body", utf8str, enc);
		
        // initialize the change object
        OSyncChange *chg = osync_change_new();
        osync_change_set_uid(chg, uid.local8Bit());
        osync_change_set_member(chg, member);

        // object type and format
        osync_change_set_objtype_string(chg, "note");
        osync_change_set_objformat_string(chg, "xml-note");
        osync_change_set_data(chg, (char*)doc, sizeof(doc), 1);

		osync_debug("knotes", 4, "Reporting note:\%s", osync_change_get_printable(chg));

        // Use the hash table to check if the object
        // needs to be reported
        osync_change_set_hash(chg, hash.data());
        if (osync_hashtable_detect_change(hashtable, chg)) {
            osync_context_report_change(ctx, chg);
            osync_hashtable_update_hash(hashtable, chg);
        }
    }

    osync_hashtable_report_deleted(hashtable, ctx, "note");

	osync_trace(TRACE_EXIT, "%s", __func__);
    return true;
}

/** basic access method
 *
 * This method is used by both access() and commit_change() method,
 * so it shouldn't call osync_context_report_success(). On success,
 * it should just return true and let the caller report success() to
 * OpenSync
 */
bool KNotesDataSource::__access(OSyncContext *ctx, OSyncChange *chg)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, chg);
    OSyncChangeType type = osync_change_get_changetype(chg);

    QString uid = osync_change_get_uid(chg);

    if (type != CHANGE_DELETED) {

        // Get osxml data
        xmlDoc *doc = (xmlDoc*)osync_change_get_data(chg);
        
        xmlNode *root = xmlDocGetRootElement(doc);
		if (!root) {
            osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
            osync_trace(TRACE_EXIT_ERROR, "%s: Invalid data", __func__);
			return false;
        }
		
		if (xmlStrcmp((root)->name, (const xmlChar *) "Note")) {
            osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Wrong root element");
            osync_trace(TRACE_EXIT_ERROR, "%s: Invalid data2", __func__);
			return false;
        }

        osync_trace(TRACE_INTERNAL, "Getting note %s and %s\n", osync_change_get_printable(chg), osxml_find_node(root, "Summary"));
        QString summary = QString(osxml_find_node(root, "Summary"));
        QString body = osxml_find_node(root, "Body");

        QString hash;
        switch (type) {
            case CHANGE_ADDED:
            	osync_trace(TRACE_INTERNAL, "addding new \"%s\" and \"%s\"\n", (const char*)summary.local8Bit(), (const char*)body.local8Bit());
            	uid = kn_iface->newNote(summary, body);
				if (kn_iface->status() != DCOPStub::CallSucceeded) {
					osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to add new note");
            		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to add new note", __func__);
					return false;
				}
				
				kn_iface->hideNote(uid);
				if (kn_iface->status() != DCOPStub::CallSucceeded)
					osync_trace(TRACE_INTERNAL, "ERROR: Unable to hide note");
				
                osync_change_set_uid(chg, uid);
                osync_change_set_hash(chg, hash);
				break;
            case CHANGE_MODIFIED:
				kn_iface->setName(uid, summary);
				if (kn_iface->status() != DCOPStub::CallSucceeded) {
					osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to set name");
					osync_trace(TRACE_EXIT_ERROR, "%s: Unable to set name", __func__);
					return false;
				}
				
				kn_iface->setText(uid, body);
				if (kn_iface->status() != DCOPStub::CallSucceeded) {
					osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to set text");
					osync_trace(TRACE_EXIT_ERROR, "%s: Unable to set text", __func__);
					return false;
				}
                hash = summary + body;
                osync_change_set_hash(chg, hash);
				break;
            default:
                osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Invalid change type");
                osync_trace(TRACE_EXIT_ERROR, "%s: Invalid change type", __func__);
				return false;
        }
    } else {
		system("dcop knotes KNotesIface hideAllNotes");
		QString asdasd = "dcop knotes KNotesIface killNote " + uid + " true";
		system((const char*)asdasd.local8Bit());
        osync_debug("knotes", 4, "Deleting note %s", (const char*)uid.local8Bit());
		/*kn_iface->killNote(uid, true);
		if (kn_iface->status() != DCOPStub::CallSucceeded) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to delete note");
			osync_trace(TRACE_EXIT_ERROR, "%s: Unable to delete note", __func__);
			return false;
		}*/
    }

	osync_trace(TRACE_EXIT, "%s", __func__);
    return true;
}

bool KNotesDataSource::commit_change(OSyncContext *ctx, OSyncChange *chg)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, chg);
    if (!__access(ctx, chg)) {
        osync_trace(TRACE_EXIT_ERROR, "%s: Unable to delete note", __func__);
		return false;
    }
    osync_hashtable_update_hash(hashtable, chg);
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
    return true;
}

bool KNotesDataSource::access(OSyncContext *ctx, OSyncChange *chg)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, chg);
	if (!__access(ctx, chg)) {
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to delete note", __func__);
		return false;
    }
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return true;
}
