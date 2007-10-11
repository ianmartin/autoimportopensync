/*
 * Copyright (C) 2006 Michael Kolmodin
 * Copyright (C) 2007 Michael Unterkalmsteiner, <michael.unterkalmsteiner@stud-inf.unibz.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SMCSYNCSOURCE_H_
#define SMCSYNCSOURCE_H_

#include <opensync/opensync.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-context.h>
#include <opensync/opensync-helper.h>

#include <spds/SyncSource.h>


#define MSGLENGTH	256 

class SyncSourceException
{

protected: 
	char msg[MSGLENGTH];

public:
	SyncSourceException( char* why );
    char* getMsg();
    
};


class SmcSyncSource : public SyncSource 
{
	
public:
	
	SmcSyncSource(SyncSourceConfig* sc);
	SmcSyncSource(const SmcSyncSource& other);
	~SmcSyncSource();
	
	SmcSyncSource& operator=(const SmcSyncSource& other);
	void setOsyncEnvironment(OSyncObjTypeSink* sink, OSyncObjFormat* objformat);
	osync_bool isOSyncSlow();
	void setOSyncSlow(osync_bool slow);
	void setSinkAvailable(osync_bool available = TRUE);
	void setIsSynced(bool synced);
	bool getIsSynced();
	void reportChanges(OSyncContext* ctx);
	void commitChange(OSyncChange* change)
	              throw(SyncSourceException);
	void updateAnchors();
	char* getLastAnchor();
	char* getNextAnchor();
	void clearAllItems();
	int numberOfAllItems() { return allItems.items.size(); }
	int numberOfNewItems() { return newItems.items.size(); }
	int numberOfUpdatedItems() { return updatedItems.items.size(); }
	int numberOfDeletedItems() { return deletedItems.items.size(); }
	const char* getObjectFormat();
		
	
	//SyncSource interface implementation
	SyncItem* getFirstItem() { return getFirst(allItems); }
	SyncItem* getNextItem() { return getNext(allItems); }
	SyncItem* getFirstNewItem() { return getFirst(newItems); }
	SyncItem* getNextNewItem() { return getNext(newItems); }
	SyncItem* getFirstUpdatedItem() { return getFirst(updatedItems); }
	SyncItem* getNextUpdatedItem() { return getNext(updatedItems); }
	SyncItem* getFirstDeletedItem() { return getFirst(deletedItems); }
	SyncItem* getNextDeletedItem() { return getNext(deletedItems); }
	SyncItem* getFirstItemKey() { return getFirst(allItems); }
	SyncItem* getNextItemKey() { return getNext(allItems); }
	void setItemStatus(const char* key, int status);
	int addItem(SyncItem& item);
	int updateItem(SyncItem& item);
	int deleteItem(SyncItem& item);
	ArrayElement* clone();
	
protected:
	
	void reportChangeItems(ArrayList& items,
	                       OSyncChangeType changeType,
	                       OSyncContext* ctx);
	
	virtual OSyncChange* convert(SyncItem* item, OSyncChangeType changeType);
	virtual SyncItem* convert(OSyncChange* change);
	
private:
		
	struct ItemIteratorContainer {
	        ArrayList items;
	        int index;
	} allItems, newItems, updatedItems, deletedItems;
	
	struct OSyncEnvironment {
		OSyncObjTypeSink*	sink;
		OSyncObjFormat* 	objformat;
	} osyncEnv;
	
	bool isSynced;
		
	SyncItem* getFirst(ItemIteratorContainer& container);
	SyncItem* getNext(ItemIteratorContainer& container);	
};



#endif /*SMCSYNCSOURCE_H_*/
