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
 * TODO: Check how notes are stored on KDE 3.3/3.4, and use the right interface.
 * (http://www.opensync.org/ticket/34)
 */



#include "knotes.h"


KNotesDataSource::KNotesDataSource(OSyncMember *m, OSyncHashTable *h)
    :member(m), hashtable(h)
{
}

bool KNotesDataSource::connect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	//check knotes running
		
	
	//connect to dcop
	DCOPClient *kn_dcop = KApplication::kApplication()->dcopClient();
	if (!kndcop) {
		osync_context_report_error(ctx, OSYNC_ERROR_INITIALIZATION, "Unable to make new dcop for knotes");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to make new dcop for knotes", __func__);
		return FALSE;
	}
	
	if (!dcop->attach()) {
		osync_context_report_error(ctx, OSYNC_ERROR_INITIALIZATION, "Unable to attach dcop for knotes");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to attach dcop for knotes", __func__);
		return FALSE;
	}
	
	QCStringList apps = dcop->registeredApplications();
	if (!apps.contains("knotes")) {
		//start knotes if not running
        knotesWasRunning = false;
	} else
        knotesWasRunning = true;

	kn_iface = new KNotesIface_stub(knotesAppname,"KNotesIface");

	osync_trace(TRACE_EXIT, "%s", __func__);
	return true;
}

bool KNotesDataSource::disconnect(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	//call sync on knotes dcop
	
	// FIXME: ugly, but necessary
	if (!knotesWasRunning) {
		//Terminate knotes
	}
	
	//detach dcop
	//destroy dcop
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return true;
}

bool KNotesDataSource::get_changeinfo(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);
	QMap <KNoteID_t,QString> fNotes;
	
	fNotes = fKNotes->notes();
	if (fKNotes->status() != DCOPStub::CallSucceeded) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unable to get changed notes");
		osync_trace(TRACE_EXIT_ERROR, "%s: Unable to get changed notes", __func__);
		return FALSE;
	}
	
	QMap<KNoteID_t,QString>::ConstIterator i = fNotes.begin();
	while (i != fNotes.end()) {
		osync_debug("knotes", 4, "Note key: %s", (const char*)i.key().local8Bit());
        osync_debug("knotes", 4, "Note contents:\n%s\n====", (const char*)i.data().local8Bit());
		
        QString uid = (*i)->uid();

        // Create osxml doc containing the note
        xmlDoc *doc = xmlNewDoc((const xmlChar*)"1.0");
        xmlNode *root = osxml_node_add_root(doc, "note");

        OSyncXMLEncoding enc;
        enc.encoding = OSXML_8BIT;
        enc.charset = OSXML_UTF8;

        // Set the right attributes
        xmlNode *sum = xmlNewChild(root, NULL, (const xmlChar*)"", NULL);
        QCString utf8str = (*i)->summary().utf8();
        osxml_node_set(sum, "Summary", utf8str, enc);

        xmlNode *body = xmlNewChild(root, NULL, (const xmlChar*)"", NULL);
        utf8str = (*i)->description().utf8();
        osxml_node_set(body, "Body", utf8str, enc);

        // initialize the change object
        OSyncChange *chg = osync_change_new();
        osync_change_set_uid(chg, uid.local8Bit());
        osync_change_set_member(chg, member);

        // object type and format
        osync_change_set_objtype_string(chg, "note");
        osync_change_set_objformat_string(chg, "xml-note");
        osync_change_set_data(chg, (char*)doc, sizeof(doc), 1);

        // Use the hash table to check if the object
        // needs to be reported
        osync_change_set_hash(chg, hash.data());
        if (osync_hashtable_detect_change(hashtable, chg)) {
            osync_context_report_change(ctx, chg);
            osync_hashtable_update_hash(hashtable, chg);
        }
    }

    return true;
}

bool KNotesDataSource::access(OSyncContext *ctx, OSyncChange *chg)
{
    OSyncChangeType type = osync_change_get_changetype(chg);

    QString uid = osync_change_get_uid(chg);

    if (type != CHANGE_DELETED) {

        // Get osxml data
        xmlDoc *doc = (xmlDoc*)osync_change_get_data(chg);
        xmlNode *root = osxml_node_get_root(doc, "note", NULL);
        if (!root) {
            osync_context_report_error(ctx, OSYNC_ERROR_CONVERT, "Invalid data");
            return false;
        }
        QString summary = osxml_find_node(root, "Summary");
        QString body = osxml_find_node(root, "Body");

        QString hash, uid;
        // end of the ugly-format parsing
        switch (type) {
            case CHANGE_ADDED:
                uid = newNote();
                osync_change_set_uid(chg, uid);
                osync_change_set_hash(chg, hash);
            break;
            case CHANGE_MODIFIED:
                setText
                setBody
                hash = calc_hash();
                osync_change_set_hash(chg, hash);
            break;
            default:
                osync_context_report_error(ctx, OSYNC_ERROR_NOT_SUPPORTED, "Invalid change type");
                return false;
        }
    } else {
        //dcop killNote
    }

    osync_context_report_success(ctx);
    return true;
}

bool KNotesDataSource::commit_change(OSyncContext *ctx, OSyncChange *chg)
{
    if (!access(ctx, chg))
        return false;
    osync_hashtable_update_hash(hashtable, chg);
    return true;
}
