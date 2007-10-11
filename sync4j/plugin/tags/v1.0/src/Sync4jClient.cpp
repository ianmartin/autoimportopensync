/*
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

#include "Sync4jClient.h"
#include <glib.h>

using namespace std;

Sync4jClientException::Sync4jClientException(int errNo) : error(errNo)
{
	switch(error) 
	{
		case ERR_CONFIGURATION:
			snprintf( msg, sizeof( msg ), "Configuration not readable (%d)", error );
			break;
		case ERR_PROTOCOL_ERROR :
			snprintf( msg, sizeof( msg ), "Protocol error (%d)", error );
	        break;
	    case  ERR_AUTH_NOT_AUTHORIZED :
	        snprintf( msg, sizeof( msg ), "Not authorized (%d)", error );
	        break;
	    case  ERR_AUTH_EXPIRED :
	    	snprintf( msg, sizeof( msg ), "Authorization expired (%d)", error );
	        break;
	    case  ERR_NOT_FOUND:
	    	snprintf( msg, sizeof( msg ), "Not found (%d)", error );
	        break;
	    case ERR_AUTH_REQUIRED:
	    	snprintf( msg, sizeof( msg ), "Authorization required (%d)", error );
	        break;
	    case  ERR_SERVER_FAILURE:
	    	snprintf( msg, sizeof( msg ), "Server failure (%d)", error );
	        break;
	    default:
	    	snprintf( msg, sizeof( msg ), "Unknown error (%d)", error );
	        break;
	}
}

char*
Sync4jClientException::getMsg()
{
	return msg;
}


//// PUBLIC ////

Sync4jClient::Sync4jClient(const char* configdir)
	throw(Sync4jClientException)
{
	this->configdir = g_strdup(configdir);
	this->configpath = g_strdup_printf("%s/%s", configdir, CONFIGFILE);
	direction = TO_SERVER;
	
	initialize();
}

Sync4jClient::~Sync4jClient()
{
	if(configdir)
		g_free(configdir);
	
	if(configpath)
		g_free(configpath);
	
	if(syncConfig)
		delete syncConfig;
}


int
Sync4jClient::getNumSources()
{
	return sources.size();
}

vector<SmcSyncSource>&
Sync4jClient::getSources()
{
	return sources;
}

SyncSourceConfig*
Sync4jClient::getSyncSourceConfig(int index)
{
	return syncConfig->getSyncSourceConfig(index);
}

void
Sync4jClient::addChange(OSyncChange* change, const char* currentSource)
	throw(SyncSourceException)
{
	SmcSyncSource* source = getSourceByName(currentSource);
	if(source) {
	    source->commitChange(change);
	}
	else {
		char msg[MSGLENGTH];
		snprintf(msg, sizeof(msg), "Source '%s' not found", currentSource);
		throw SyncSourceException(msg);
	}
}

void 
Sync4jClient::prepareFromServerSync() 
{
	this->direction = FROM_SERVER;
	
	if(!areAllSourcesSynced()) {
		vector<SmcSyncSource>::iterator iter;
		for(iter = sources.begin(); iter != sources.end(); iter++) {
			if(iter->getSyncMode() == SYNC_SLOW) {
				iter->setOSyncSlow(TRUE);
				LOG.debug("Sync4j requested a slow sync for source '%s'", iter->getName());
			}
			else if(iter->isOSyncSlow()) {
				iter->setPreferredSyncMode(SYNC_SLOW);
				LOG.debug("Opensync requested a slow sync for source '%s'", iter->getName());
			}
			else {
				iter->setPreferredSyncMode(SYNC_ONE_WAY_FROM_SERVER);
				iter->setOSyncSlow(FALSE);
				LOG.debug("Making a one-way from server sync for source '%s'", iter->getName());
			}
		}
	}
}

void
Sync4jClient::endFromServerSync(OSyncContext* ctx, const char* currentSource) 
{
	SmcSyncSource* source = getSourceByName(currentSource);
	if(source) {
		LOG.debug("Reporting changes for source '%s'", source->getName());
		source->reportChanges(ctx);
		source->updateAnchors();
		source->clearAllItems();
	}
}

void
Sync4jClient::prepareToServerSync(const char* currentSource)
{
	this->direction = TO_SERVER;
	
	SmcSyncSource* source = getSourceByName(currentSource);
	if(source) {
		if(source->numberOfAllItems()) {
			if(source->isOSyncSlow()) {
				source->setPreferredSyncMode(SYNC_SLOW);
				LOG.debug("Opensync requested a slow sync for source '%s'", source->getName());
			}
			else {
				source->setPreferredSyncMode(SYNC_ONE_WAY_FROM_CLIENT);
				LOG.debug("Making a one-way from client sync for source '%s'", source->getName());
			}
			
			LOG.debug("%d items to sync", source->numberOfAllItems());
			LOG.debug("%d new items to sync", source->numberOfNewItems());
			LOG.debug("%d updated items to sync", source->numberOfUpdatedItems());
			LOG.debug("%d deleted items to sync", source->numberOfDeletedItems());
		}
		else {
			source->getReport()->setState(SOURCE_INACTIVE);
			LOG.debug("Source '%s' set inactive because it has no items", source->getName());
		}
		
		source->setIsSynced(false);
	}
}


void
Sync4jClient::sync()
	throw(Sync4jClientException)
{
	int ret = ERR_UNSPECIFIED;
		
	if(direction == FROM_SERVER)
		ret = syncFromServer();
	else if(direction == TO_SERVER)
		ret = syncToServer();
	
	if(ret)
		throw Sync4jClientException(ret);
}


void
Sync4jClient::updateOSyncAnchor(const char* currentSource)
{
	SmcSyncSource* source = getSourceByName(currentSource);
	if(source) {
		char* anchorpath = g_strdup_printf("%s/anchor.db", configdir);
		char* anchor = source->getNextAnchor();
		osync_anchor_update(anchorpath, source->getName(), anchor);
		g_free(anchorpath);
		delete [] anchor;
	}
}

void 
Sync4jClient::saveConfig()
{
	syncConfig->save();
}

//// PROTECTED ////

SmcSyncSource*
Sync4jClient::getSourceByName(const char* name)
{
	vector<SmcSyncSource>::iterator iter;
	for(iter = sources.begin(); iter != sources.end(); iter++) {
		if(strcmp(iter->getName(), name) == 0) {
			/* iter is *not* a pointer to the element so return &iter obviously
			 * doesn't work. First dereference, then get the address; indexed access
			 * would avoid that syntax...
			 */
			return &(*iter);
		}
	}
	
	return NULL;
}



//// PRIVATE ////

void
Sync4jClient::initialize()
	throw(Sync4jClientException)
{
	LOG.debug("Configuration is located at %s", configpath);
	syncConfig = new DMTClientConfig(configpath, "sync4jconfig");
	
	int numSources = 0;
	if(syncConfig->read())
	{
		numSources = syncConfig->getSyncSourceConfigsCount();
		LOG.debug("Successfully read configuration. %d sources found", numSources);
	}
	else 
	{
		delete syncConfig;
		syncConfig = NULL;
		throw Sync4jClientException(ERR_CONFIGURATION);
	}
			
	for(int i = 0; i < numSources; i++)
	{
		SyncSourceConfig* sc = syncConfig->getSyncSourceConfig(i);
		SmcSyncSource* ssc = new SmcSyncSource(sc);
		sources.push_back(*ssc);
		LOG.debug("Created sync source '%s'", ssc->getName());
	}
}




int
Sync4jClient::syncFromServer()
{
	int ret = ERR_UNSPECIFIED;
	
	if(areAllSourcesSynced()) {
		ret = ERR_NONE;
		LOG.debug("No synchronization. Using cached results.");
	}
	else {
		SyncSource** sourcesArray = toArray();
		ret = SyncClient::sync(*syncConfig, sourcesArray);
		delete [] sourcesArray;
		
		vector<SmcSyncSource>::iterator iter;				
		for(iter = sources.begin(); iter != sources.end(); iter++ )	
			iter->setIsSynced(true);
	
		StringBuffer buf;
		syncReport.toString(buf, true);
		LOG.info("\n%s", buf.c_str());
	}
			
	return ret;
}

int 
Sync4jClient::syncToServer()
{
	int ret = ERR_UNSPECIFIED;
	
	if(areAllSourcesReadyToSync()) {
		if(isAnyActiveSource()) {
			SyncSource** sourcesArray = toArray();
			ret = SyncClient::sync(*syncConfig, sourcesArray);
			delete [] sourcesArray;
			
			StringBuffer buf;
			syncReport.toString(buf, true);
			LOG.info("\n%s", buf.c_str());
		}
		else {
			ret = ERR_NONE;
			LOG.info("No synchronization performed because there are no local changes");
		}
		
	}
	else {
		ret = ERR_NONE;
		LOG.debug("No synchronization. Waiting til all sources are committed");
	}
		
	return ret;
}

bool 
Sync4jClient::areAllSourcesSynced() 
{
	vector<SmcSyncSource>::iterator iter;
	for(iter = sources.begin(); iter != sources.end(); iter++) {
		if(!iter->getIsSynced())
			return false;
	}
	
	return true;
}

bool 
Sync4jClient::areAllSourcesReadyToSync() 
{
	vector<SmcSyncSource>::iterator iter;
	for(iter = sources.begin(); iter != sources.end(); iter++) {
		if(iter->getIsSynced())
			return false;
	}
		
	return true;
}

bool
Sync4jClient::isAnyActiveSource() 
{
	vector<SmcSyncSource>::iterator iter;
	for(iter = sources.begin(); iter != sources.end(); iter++) {
		if(iter->getReport()->getState() == SOURCE_ACTIVE)
			return true;
	}
	
	return false;
}

SyncSource**
Sync4jClient::toArray() 
{
	SyncSource** sourcesArray = NULL;
		
	sourcesArray = new SyncSource* [sources.size() + 1];
		
	for(unsigned int i = 0; i < sources.size(); i++) 
		sourcesArray[i] = &sources[i];
	
	sourcesArray[sources.size()] = NULL;
	
	return sourcesArray;
}

