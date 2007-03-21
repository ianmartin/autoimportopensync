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
#include <kabc/addressbook.h>
#include <kabc/vcardconverter.h>
#include <kabc/resource.h>

#include "osyncbase.h"

namespace KABC
{
class Addressee;
class Address;
class Ticket;
}

using namespace KABC;

class KContactDataSource
{
	private:
		AddressBook* addressbookptr;
		// for a local file, we need to obtain a saveTicket when opening the abook
		Ticket*ticket;
		//help to only save when when changed
		bool abChanged;

		KABC::VCardConverter* converter;

		OSyncHashTable *hashtable;
		OSyncMember *member;

		bool __vcard_access(OSyncContext *ctx, OSyncChange *chg);
		OSyncChange *_addressee_to_change(Addressee *a);

	public:
		KContactDataSource(OSyncMember *member, OSyncHashTable *hashtable);
		~KContactDataSource();

		QString calc_hash(Addressee &e);
		bool connect(OSyncContext *ctx);
		bool disconnect(OSyncContext *ctx);
		bool contact_get_changeinfo(OSyncContext *ctx);
		bool vcard_access(OSyncContext *ctx, OSyncChange *chg);
		bool vcard_commit_change(OSyncContext *ctx, OSyncChange *chg);
		bool connected;
};

#endif

