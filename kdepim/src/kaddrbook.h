/***********************************************************************
MultiSync Plugin for KDE 3.x
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

#ifndef KADDRBOOK_H
#define KADDRBOOK_H

#include <kabc/stdaddressbook.h>
#include <kabc/vcardconverter.h>
#include <kabc/resource.h>

#include "osyncbase.h"

class KContactDataSource
{
	private:
		KABC::AddressBook* addressbookptr;
		OSyncHashTable *hashtable;

		bool __vcard_access(OSyncContext *ctx, OSyncChange *chg);


	public:
		KContactDataSource(OSyncHashTable *hashtable);

		QString calc_hash(KABC::Addressee &e);
		bool connect(OSyncPluginInfo *info, OSyncContext *ctx);
		bool disconnect(OSyncPluginInfo *info, OSyncContext *ctx);
		bool contact_get_changeinfo(OSyncPluginInfo *info, OSyncContext *ctx);
		bool vcard_access(OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *chg);
		bool vcard_commit_change(OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *chg);
		bool connected;
};

#endif

