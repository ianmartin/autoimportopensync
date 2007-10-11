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


#ifndef SYNC4JCLIENT_H_
#define SYNC4JCLIENT_H_

#include <vector>
#include <client/SyncClient.h>
#include <client/DMTClientConfig.h>

#include "SmcSyncSource.h"

#define ERR_CONFIGURATION	300
#define CONFIGFILE			"syncml-client.conf"

using namespace std;

typedef enum SyncDirection
{
	TO_SERVER,
	FROM_SERVER
} SyncDirection;

class Sync4jClientException
{

protected:
	char msg[MSGLENGTH];

public:
	const int error;
	Sync4jClientException(const int errNo);
	char* getMsg();
};

class Sync4jClient : public SyncClient
{
	
private:
	char* configdir;
	char* configpath;
	DMTClientConfig* syncConfig;
	vector<SmcSyncSource> sources;
	SyncDirection direction;
	
	void initialize()
		throw(Sync4jClientException);
	int syncFromServer();
	int syncToServer();
	bool areAllSourcesSynced();
	bool areAllSourcesReadyToSync();
	bool isAnyActiveSource();
	SyncSource** toArray();
	
protected:
	SmcSyncSource* getSourceByName(const char* name);
	
public:
	
	Sync4jClient(const char* configdir)
		throw(Sync4jClientException);
	~Sync4jClient();
	int getNumSources();
	vector<SmcSyncSource>& getSources();
	SyncSourceConfig* getSyncSourceConfig(int index);
	void addChange(OSyncChange* change, const char* currentSource)
		throw(SyncSourceException);
	void prepareFromServerSync();
	void endFromServerSync(OSyncContext* ctx, const char* currentSource);
	void prepareToServerSync(const char* currentSource);
	void sync()
		throw(Sync4jClientException);
	void updateOSyncAnchor(const char* currentSource);
	void saveConfig();
	
};


#endif /*SYNC4JCLIENT_H_*/
