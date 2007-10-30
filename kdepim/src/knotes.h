/***********************************************************************
KNotes OSyncDataSource class
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

#include <kglobal.h>
#include <kstandarddirs.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kapplication.h>
#include <qmap.h>
#include "KNotesIface.h"
#include "KNotesIface_stub.h"
#include <stdio.h>
#include <qtimer.h>
#include <dcopclient.h>
#include <qstring.h>
#include <qstringlist.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef QString KNoteID_t;

#include "osyncbase.h"
#include "datasource.h"

/** KNotes access implementation interface
 */
class KNotesDataSource : public OSyncDataSource
{
	private:
		DCOPClient *kn_dcop;
		KNotesIface_stub *kn_iface;

		/** Ugly hack to restart KNotes if it was running */
		bool knotesWasRunning;

		bool saveNotes(OSyncContext *ctx);
	
public:
		KNotesDataSource() : OSyncDataSource("note") {};
		virtual ~KNotesDataSource() {};

		bool initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error);
		virtual void connect(OSyncPluginInfo *info, OSyncContext *ctx);
		virtual void disconnect(OSyncPluginInfo *info, OSyncContext *ctx);
		virtual void get_changes(OSyncPluginInfo *info, OSyncContext *ctx);
		virtual void commit(OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *chg);
};
