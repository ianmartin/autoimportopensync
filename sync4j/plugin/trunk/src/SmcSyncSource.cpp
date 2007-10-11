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

#include "SmcSyncSource.h"

SyncSourceException::SyncSourceException( char* why )
{
	strncpy( msg, why, sizeof( msg ) );
}

char*
SyncSourceException::getMsg()
{
    return msg;
}

SmcSyncSource::SmcSyncSource(SyncSourceConfig* sc): 
	SyncSource(sc->getName(), sc),
	isSynced(false)
{
	osyncEnv.sink = NULL;
	osyncEnv.objformat = NULL;
}

SmcSyncSource::SmcSyncSource(const SmcSyncSource& other) : 
	SyncSource(other),
	allItems(other.allItems),
	newItems(other.newItems),
	updatedItems(other.updatedItems),
	deletedItems(other.deletedItems),
	osyncEnv(other.osyncEnv), 
	isSynced(other.isSynced) 
{
	if(osyncEnv.sink)
		osync_objtype_sink_ref(osyncEnv.sink);
}

SmcSyncSource::~SmcSyncSource()
{
	if(osyncEnv.sink)
		osync_objtype_sink_unref(osyncEnv.sink);
}

SmcSyncSource&
SmcSyncSource::operator=(const SmcSyncSource& other) 
{
	if(this != &other) {
		SyncSource::operator=(other);
		
		allItems = other.allItems;
		newItems = other.newItems;
		updatedItems = other.updatedItems;
		deletedItems = other.deletedItems;
		osyncEnv = other.osyncEnv;
		isSynced = other.isSynced;
		
		osync_objtype_sink_ref(osyncEnv.sink);
	}
	
	return *this;
}

void 
SmcSyncSource::setOsyncEnvironment(OSyncObjTypeSink* sink, OSyncObjFormat* objformat) 
{
	osyncEnv.sink = sink;
	osyncEnv.objformat = objformat;
	
	osync_objtype_sink_ref(osyncEnv.sink);
}

osync_bool
SmcSyncSource::isOSyncSlow() 
{
	return osync_objtype_sink_get_slowsync(osyncEnv.sink);
}

void
SmcSyncSource::setOSyncSlow(osync_bool slow) 
{
	osync_objtype_sink_set_slowsync(osyncEnv.sink, slow);
}

void
SmcSyncSource::setSinkAvailable(osync_bool available)
{
	osync_objtype_sink_set_available(osyncEnv.sink, available);
}

void
SmcSyncSource::setIsSynced(bool synced) 
{
	isSynced = synced;
}

bool
SmcSyncSource::getIsSynced()
{
	return isSynced;
}

void 
SmcSyncSource::reportChangeItems(ArrayList& items, 
                                 OSyncChangeType changeType,
                                 OSyncContext* ctx)
{
    OSyncChange* change;
    int size = items.size();
    
    for(int i = 0; i < size; i++) {
        change = convert((SyncItem*)items[ i ], changeType);
        
        if(change) {
        	LOG.info("Submitting change, data:\n%s\n",
        				osync_data_get_printable(osync_change_get_data(change)));
        	
        	osync_context_report_change(ctx, change); 
        }
        else
        	LOG.error("Could not create osync change in source '%s'", getName());
    }
}

/**
*
* Report all detected changes to opensync engine.
*
*  @param ctx   Used when reporting changes  through 
*               osync_context_report_change().
*  @param changeFactory Creates changes reported to opensync.
*
*/
void SmcSyncSource::reportChanges( OSyncContext* ctx )
{
   reportChangeItems( updatedItems.items, OSYNC_CHANGE_TYPE_MODIFIED, ctx );
        
   reportChangeItems( deletedItems.items, OSYNC_CHANGE_TYPE_DELETED, ctx );
        
   reportChangeItems( newItems.items, OSYNC_CHANGE_TYPE_ADDED, ctx );
   
   LOG.info("Reported %d new items", newItems.items.size());
   LOG.info("Reported %d deleted items", deletedItems.items.size());
   LOG.info("Reported %d updated items", updatedItems.items.size());
}

/**
*
*  Accept a change to be forwarded to the remote side during sync().
*  Note that prepareForSync should be invoked after last
*  call to this method but the before the sync() call.
*
*  @param itemFactory Used to convert the change to a SyncItem 
*  @param change - the change to handleA
*
*
*/
void 
SmcSyncSource::commitChange(OSyncChange* change)
    throw( SyncSourceException )
{

	SyncItem* item;
	
    /*osync_debug("SYNCML-CLIENT", 3, "Writing change %s with changetype %i", 
                 osync_change_get_uid(change), 
                 osync_change_get_changetype(change) );*/

    switch( osync_change_get_changetype(change) ) {
        case OSYNC_CHANGE_TYPE_DELETED:
            item = convert(change);
        	allItems.items.add( *item );
        	deletedItems.items.add( *item );
            break;
        case OSYNC_CHANGE_TYPE_ADDED:
        	item = convert(change);
        	allItems.items.add( *item );
        	newItems.items.add( *item );
        	break;
        case OSYNC_CHANGE_TYPE_MODIFIED:
        	item = convert(change);
        	allItems.items.add( *item );
        	updatedItems.items.add( *item );
        	break;
        default:
            char msg[MSGLENGTH];
            snprintf( msg, sizeof( msg ),  "Unknown change type, %d", 
                      osync_change_get_changetype(change) );
            throw SyncSourceException( msg );
            break;
    }
}

SyncItem* 
SmcSyncSource::convert(OSyncChange* change) 
{
	const char* key = osync_change_get_uid(change);
	SyncItem* syncItem = NULL;
		
	if(key) {
		syncItem = new SyncItem(key);
			
		OSyncData* osData = osync_change_get_data(change); 
		if(osync_data_has_data(osData)){
			char* data = NULL;
			unsigned int size = 0;
			osync_data_get_data(osData, &data, &size);
			syncItem->setData(data, size * sizeof(char));
			       
			LOG.info("Creating SyncItem, size: %ld, data:\n%s\n", 
					syncItem->getDataSize(), (char*) syncItem->getData());
	    }
	    syncItem->setDataType(config.getType());
	}
	
	return syncItem;
}

OSyncChange* 
SmcSyncSource::convert(SyncItem* item, OSyncChangeType changeType)
{
	//TODO throw a SyncSourceException on error
	
	OSyncError* error = NULL;
    OSyncChange* change = NULL;
    change = osync_change_new(&error);
    
    if(change) {
    	        
        if( changeType != OSYNC_CHANGE_TYPE_DELETED ) {
        	OSyncData* osData = NULL;
        	char* data = (char*) item->getData();
        	unsigned int len = strlen(data);
        	        	
        	if(osyncEnv.objformat) {
        		osData = osync_data_new(data, len, osyncEnv.objformat, &error);
        		if(osData) {
        			osync_data_set_objtype(osData, getName());
        			osync_change_set_data(change, osData);
        			osync_change_set_uid(change, item->getKey());
        			osync_change_set_changetype(change, changeType);
        			osync_data_unref(osData);
        		}
        		else {
        			LOG.error("SmcSyncSource::convert -- %s", osync_error_print(&error));
        		}
        	}
        	else {
        		LOG.error("SmcSyncSource::convert -- %s", osync_error_print(&error));
        	}    
        }
    }
    else {
    	LOG.error("SmcSyncSource::convert -- %s", osync_error_print(&error));
    }
    
    osync_error_unref(&error);

    return change;
}

/* 
*
* Update anchors: lastAnchor = nextAnchor, nextAnchor = <a new value>
*
*/
void 
SmcSyncSource::updateAnchors()
{
    const char* anchor;
    char newAnchor[ DIM_ANCHOR ];

    anchor = SyncSource::getNextAnchor();
    SyncSource::setLastAnchor( anchor ); 
    long now = time( NULL );
        
    snprintf( newAnchor, DIM_ANCHOR, "%u", now );
    
    while( strcmp( anchor, newAnchor) == 0 ){
        // We happened to get the same second as previous one...
        now += 1;
        snprintf( newAnchor, DIM_ANCHOR, "%u", now );
    }

    SyncSource::setNextAnchor( newAnchor );
}

/**
*
* Return last anchor, a char* allocated with new[]
*
*/
char* 
SmcSyncSource::getLastAnchor()
{
	char* buf;
	const char* last = SyncSource::getLastAnchor();
	size_t len = strlen(last);
	buf = new char[len + 1];
	strcpy(buf, last);
   
	return buf;
}

/**
*
* Return the Next anchor, a char* allocated with new[]
*
*/
char* 
SmcSyncSource::getNextAnchor()
{
	char* buf;
	const char* next = SyncSource::getNextAnchor();
	size_t len = strlen(next);
	buf = new char[len + 1];
	strcpy(buf, next);
	
	return buf;
}

void
SmcSyncSource::clearAllItems()
{
	allItems.items.clear();
	newItems.items.clear();
	updatedItems.items.clear();
	deletedItems.items.clear();
}

// TODO don't check against objType but mimetype
const char*
SmcSyncSource::getObjectFormat()
{
	if(strcmp("contact", getName()) == 0)
		return "vcard21";
	else if(strcmp("event", getName()) == 0)
		return "vevent10";
	else if(strcmp("todo", getName()) == 0)
		return "sift";
	else if(strcmp("note", getName()) == 0)
		return "sifn";
	else
		return NULL;
}

SyncItem*
SmcSyncSource::getFirst(ItemIteratorContainer& container)
{
	container.index = 0;
	if(container.index >= container.items.size()) 
	{
		return NULL;
	}
	
	return (SyncItem*)container.items.get(container.index)->clone();
}

SyncItem*
SmcSyncSource::getNext(ItemIteratorContainer& container)
{
	 container.index++;
	 if (container.index >= container.items.size()) 
	 {
		 return NULL;
	 }
	 
	 return (SyncItem*)container.items.get(container.index)->clone();
}



void
SmcSyncSource::setItemStatus(const char* key, int status)
{
	/* maybe here we can delete the item from the list, if status says that the
	 * item was successfully transfered to the server. See API doc
	 * This would make the call to clearAllItems() after synchronization superfluous.
	 */
}

int
SmcSyncSource::addItem(SyncItem& item)
{
	int ret = STC_COMMAND_FAILED;
	
	if((allItems.items.add(item) != -1) && (newItems.items.add(item) != -1))
		ret = STC_ITEM_ADDED;
	
	return ret;
}

int
SmcSyncSource::updateItem(SyncItem& item)
{
	int ret = STC_COMMAND_FAILED;
		
	if((allItems.items.add(item) != -1) && (updatedItems.items.add(item) != -1))
		ret = STC_OK;
		
	return ret;
}

int
SmcSyncSource::deleteItem(SyncItem& item)
{
	int ret = STC_COMMAND_FAILED;
		
	if((allItems.items.add(item) != -1) && (deletedItems.items.add(item) != -1))
		ret = STC_OK;
		
	return ret;
}

ArrayElement*
SmcSyncSource::clone()
{
	SmcSyncSource* s = new SmcSyncSource(&(getConfig()));
	//TODO does assign() do enough??
	s->assign(*this);

	return s;
}

