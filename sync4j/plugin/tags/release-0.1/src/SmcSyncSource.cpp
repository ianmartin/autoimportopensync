/*
 * Copyright (C) 2006 Michael Kolmodin
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

#include <SmcSyncSource.h>

SyncSourceException::SyncSourceException( char* why )
{
	strncpy( msg, why, sizeof( msg ) );
}
char*  SyncSourceException::getMsg()
{
    return msg;
}

/**
* An interface class, hiding the syncml API strange things from
* opensync.
*
*/

/* 
*
* Update anchors: lastAnchor = nextAnchor, nextAnchor = <a new value>
*
*/

void SmcSyncSource::updateAnchors()
{
    wchar_t anchor[ DIM_ANCHOR ];
    wchar_t newAnchor[ DIM_ANCHOR ];

    SyncSource::getNextAnchor( anchor );
    SyncSource::setLastAnchor( anchor );
    LOG.setMessage( L"New last anchor: %ls", anchor); LOG.debug();
    long now = time( NULL );
    swprintf( newAnchor, DIM_ANCHOR, L"%lu", time( NULL ) );
    while( wcscmp( anchor, newAnchor) == 0 ){
        // We happened to get the same second as previous one...
        now += 1;
        swprintf( newAnchor, DIM_ANCHOR, L"%lu", time( NULL ) );
    }

    SyncSource::setNextAnchor( newAnchor );
}

/**
*
* Return last anchor, a char* allocated with new[]
*
*/
char* SmcSyncSource::getLastAnchor()
{
    return new_wcstombs( SyncSource::getLastAnchor( NULL ) );
}

/**
*
* Return the Next anchor, a char* allocated with new[]
*
*/
char* SmcSyncSource::getNextAnchor()
{
    return new_wcstombs( SyncSource::getNextAnchor( NULL ) );
}

/**
* Create a SyncSource.
*
*  @return A newly allocedet SyncSource, or 0. In the latter
*          case, lastErrorCode reflects the reason.
*
*/
SmcSyncSource* createSyncSource( char* key )
    throw( ConfigException )
{
    SmcSyncSource* syncSource;

    wchar_t* wcsKey = new_mbsrtowcs( key );
    lastErrorCode = 0;
    syncSource = new SmcSyncSource( wcsKey );
    delete[] wcsKey;
    if( lastErrorCode != 0 )
        throw ConfigException( "Cannot create source (bad gconf uri?)" );
    
    return syncSource;
}

SmcSyncSource::SmcSyncSource( wchar_t* key ): SyncSource( key )
{
}

SmcSyncSource::~SmcSyncSource()
{
}

void SmcSyncSource::reportChangeItems( SyncItem**            items, 
                                       size_t                size, 
                                       OSyncChangeType       changeType,
                                       SmcChangeFactory*     changeFactory,
                                       OSyncContext*         ctx  )
{
    OSyncChange* change;

    for( unsigned int i = 0; i < size; i += 1 ){
        change = changeFactory->getChange( items[ i ], changeType );
        LOG.setMessage( L"Submitting change, data:\n%s\n, size:%d",
                        osync_change_get_data( change ),
                        osync_change_get_datasize( change ) );
        LOG.debug();
        osync_context_report_change(ctx, change); 
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
void SmcSyncSource::reportChanges( OSyncContext*      ctx, 
                                   SmcChangeFactory*  changeFactory )
{
    SyncItem**     items;
    size_t         size;

    if(  getSyncMode() == SYNC_SLOW ){
        items =  getAllSyncItems();
        size = getAllSyncItemsCount();
        reportChangeItems( items, size, CHANGE_UNKNOWN, changeFactory, ctx );
    }
    else{
        items = getUpdatedSyncItems();
        size = getUpdatedSyncItemsCount();
        reportChangeItems( items, size, CHANGE_MODIFIED, changeFactory, ctx );

        items = getDeletedSyncItems();
        size = getDeletedSyncItemsCount();
        reportChangeItems( items, size, CHANGE_DELETED, changeFactory, ctx );

        items = getNewSyncItems();
        size = getNewSyncItemsCount();
        reportChangeItems( items, size, CHANGE_ADDED, changeFactory, ctx );
    }
}


/**
* Returns a classic C array allocated with new containing the
* changes in the input std::vector
*/
SyncItem** SmcSyncSource::ItemVector2Array( SyncItemVector_t changes )
{
    if( changes.size() == 0 )
        return NULL;

    SyncItem** retArray = new SyncItem*[ changes.size() + 1 ];
    for( unsigned int i = 0; i < changes.size(); i += 1 )
        retArray[ i ] = changes[ i ];
    retArray[ changes.size() ] = (SyncItem*) 0;

    return retArray;
}

/**
*   Make necessary internal housekeeping before doing a sync()
*   operation.
*/
void  SmcSyncSource::prepareForSync()
{
    SyncItem**     ms_deletions;
    SyncItem**     ms_modifications;
    SyncItem**     ms_additions;

    if( deletions.size() > 0 ){
        LOG.setMessage( L"Committing %d deletions", deletions.size() );
        LOG.debug();
        ms_deletions = ItemVector2Array( deletions );
        setDeletedSyncItems( ms_deletions, deletions.size() );
    }

    if( modifications.size() > 0 ){
        LOG.setMessage( L"Committing %d mods", modifications.size() );
        LOG.debug();
        ms_modifications = ItemVector2Array( modifications );
        setUpdatedSyncItems( ms_modifications, modifications.size() );
    }

    if( additions.size() > 0 ){
        LOG.setMessage( L"Committing %d new ones",  additions.size() );
        LOG.debug();
        ms_additions = ItemVector2Array( additions );
        setNewSyncItems( ms_additions, additions.size() );
    }
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
*  @return  TRUE if the change was accepted, else FALSE.
*
*/
void SmcSyncSource::commitChange( ItemFactory* itemFactory, 
                                  OSyncChange* change )
    throw( SyncSourceException )
{

    osync_debug("SYNCML-CLIENT", 3, "Writing change %s with changetype %i", 
                 osync_change_get_uid(change), 
                 osync_change_get_changetype(change) );

    switch( osync_change_get_changetype(change) ) {
        case CHANGE_DELETED:
                deletions.push_back( itemFactory->getItem( change ) );
                break;
        case CHANGE_ADDED:
                additions.push_back( itemFactory->getItem( change ) );
                break;
        case CHANGE_MODIFIED:
                modifications.push_back( itemFactory->getItem( change ) );
                break;
        default:
                char msg[ 256 ];
                snprintf( msg, sizeof( msg ),  "Unknown change type, %d", 
                             osync_change_get_changetype(change) );
                throw SyncSourceException( msg );
                break;
    }
}
