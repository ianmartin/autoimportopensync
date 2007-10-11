#include "Sync4jClientTest.h"

CPPUNIT_TEST_SUITE_REGISTRATION(Sync4jClientTest);

void
Sync4jClientTest::testCreateSyncSource()
{
	/*char* names[] = { "contact", "task", NULL };
	SyncManagerConfig smConfig;
	Sync4jClient client;
	
	SyncSourceConfig sc0;
	sc0.setName(names[0]);
	SyncSourceConfig sc1;
	sc1.setName(names[1]);
	
	CPPUNIT_ASSERT(smConfig.setSyncSourceConfig(sc0));
	CPPUNIT_ASSERT(smConfig.setSyncSourceConfig(sc1));
	
	int numSources = smConfig.getSyncSourceConfigsCount();
	CPPUNIT_ASSERT_EQUAL(2, numSources);
	
	SyncSource** sources = new SyncSource* [numSources + 1];
	
	for(int i = 0; i < numSources; i++)
	{
		CPPUNIT_ASSERT_EQUAL(ERR_NONE, client.createSyncSource(names[i], 
				i, smConfig.getSyncSourceConfig(i), sources));
	}
	sources[numSources] = NULL;
	
	int i = 0;
	while(sources[i])
	{
		CPPUNIT_ASSERT(strcmp(names[i], sources[i]->getName()) == 0);
		i++;
	}*/
	
}
