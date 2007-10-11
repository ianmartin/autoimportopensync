#include "SmcSyncSourceTest.h"

#include <client/DMTClientConfig.h>
#include <filter/SourceFilter.h>
#include <stdlib.h>
#include <iostream>


CPPUNIT_TEST_SUITE_REGISTRATION(SmcSyncSourceTest);

void
SmcSyncSourceTest::testConstructor()
{
	DMTClientConfig config("data/syncml-client", "sync4jconfig");
	CPPUNIT_ASSERT(config.read());
	
	SyncSourceConfig* sc = config.getSyncSourceConfig("contact");
	CPPUNIT_ASSERT(sc);
	
	SmcSyncSource s(sc);
	CPPUNIT_ASSERT(strcmp(s.getName(), "contact") == 0);
	CPPUNIT_ASSERT(s.getReport() == NULL);
	CPPUNIT_ASSERT(s.getFilter() == NULL);
	CPPUNIT_ASSERT(s.getLastSync() == 0);
	CPPUNIT_ASSERT(s.getNextSync() == 0);
	CPPUNIT_ASSERT(s.getPreferredSyncMode() == SYNC_TWO_WAY);
	CPPUNIT_ASSERT(&s.getConfig() == sc);
	
	CPPUNIT_ASSERT(s.numberOfAllItems() == 0);
	CPPUNIT_ASSERT(s.numberOfNewItems() == 0);
	CPPUNIT_ASSERT(s.numberOfUpdatedItems() == 0);
	CPPUNIT_ASSERT(s.numberOfDeletedItems() == 0);
}

void
SmcSyncSourceTest::testCopyConstructor()
{
	DMTClientConfig config("data/syncml-client", "sync4jconfig");
	CPPUNIT_ASSERT(config.read());
		
	SyncSourceConfig* sc = config.getSyncSourceConfig("contact");
	CPPUNIT_ASSERT(sc);
	
	SmcSyncSource s1(sc);
	SyncSourceReport ssr;
	s1.setReport(&ssr);
	
	s1.setPreferredSyncMode(SYNC_SLOW);
	s1.setLastSync(1234);
	s1.setNextSync(5678);
	s1.setNextAnchor("abc");
	s1.setLastAnchor("efg");
	
	SourceFilter sf;
	s1.setFilter(&sf);
	
	SyncItem item;
	
	s1.addItem(item);
	s1.addItem(item);
	s1.updateItem(item);
	s1.deleteItem(item);
	
	
	SmcSyncSource s2(s1);
		
	CPPUNIT_ASSERT(strcmp(s1.getName(), s2.getName()) == 0);
	CPPUNIT_ASSERT(s1.getReport() == s2.getReport());
	CPPUNIT_ASSERT(s1.getFilter() != s2.getFilter()); //TODO add a check for equivalence
	CPPUNIT_ASSERT(s1.getLastSync() == s2.getLastSync());
	CPPUNIT_ASSERT(s1.getNextSync() == s2.getNextSync());
	
	char* val1 = NULL;
	char* val2 = NULL;
	
	val1 = s1.getLastAnchor();
	val2 = s2.getLastAnchor();
	CPPUNIT_ASSERT(strcmp(val1, val2) == 0);
	delete [] val1;
	delete [] val2;
	
	val1 = s1.getNextAnchor();
	val2 = s2.getNextAnchor();
	CPPUNIT_ASSERT(strcmp(val1, val2) == 0);
	delete [] val1;
	delete [] val2;
	
	CPPUNIT_ASSERT(s1.getPreferredSyncMode() == s2.getPreferredSyncMode());
	CPPUNIT_ASSERT(&s1.getConfig() == &s2.getConfig());
		
	CPPUNIT_ASSERT(s1.numberOfAllItems() == s2.numberOfAllItems());
	CPPUNIT_ASSERT(s1.numberOfNewItems() == s2.numberOfNewItems());
	CPPUNIT_ASSERT(s1.numberOfUpdatedItems() == s2.numberOfUpdatedItems());
	CPPUNIT_ASSERT(s1.numberOfDeletedItems() == s2.numberOfDeletedItems());
}

void
SmcSyncSourceTest::testOperatorAssignment() 
{
	DMTClientConfig config("data/syncml-client", "sync4jconfig");
	CPPUNIT_ASSERT(config.read());
			
	SyncSourceConfig* sc = config.getSyncSourceConfig("contact");
	CPPUNIT_ASSERT(sc);
		
	SmcSyncSource s1(sc);
	SyncSourceReport ssr;
	s1.setReport(&ssr);
	
	s1.setPreferredSyncMode(SYNC_SLOW);
	s1.setLastSync(1234);
	s1.setNextSync(5678);
	s1.setNextAnchor("abc");
	s1.setLastAnchor("efg");
	
	SourceFilter sf;
	s1.setFilter(&sf);
	
	SyncItem item;
	
	s1.addItem(item);
	s1.addItem(item);
	s1.updateItem(item);
	s1.deleteItem(item);
	
	SmcSyncSource s2 = s1;
		
	
	CPPUNIT_ASSERT(strcmp(s1.getName(), s2.getName()) == 0);
	CPPUNIT_ASSERT(s1.getReport() == s2.getReport());
	CPPUNIT_ASSERT(s1.getFilter() != s2.getFilter()); //TODO add a check for equivalence
	CPPUNIT_ASSERT(s1.getLastSync() == s2.getLastSync());
	CPPUNIT_ASSERT(s1.getNextSync() == s2.getNextSync());
	
	char* val1 = NULL;
	char* val2 = NULL;
	
	val1 = s1.getLastAnchor();
	val2 = s2.getLastAnchor();
	CPPUNIT_ASSERT(strcmp(val1, val2) == 0);
	delete [] val1;
	delete [] val2;
	
	val1 = s1.getNextAnchor();
	val2 = s2.getNextAnchor();
	CPPUNIT_ASSERT(strcmp(val1, val2) == 0);
	delete [] val1;
	delete [] val2;
	
	CPPUNIT_ASSERT(s1.getPreferredSyncMode() == s2.getPreferredSyncMode());
	CPPUNIT_ASSERT(&s1.getConfig() == &s2.getConfig());
		
	CPPUNIT_ASSERT(s1.numberOfAllItems() == s2.numberOfAllItems());
	CPPUNIT_ASSERT(s1.numberOfNewItems() == s2.numberOfNewItems());
	CPPUNIT_ASSERT(s1.numberOfUpdatedItems() == s2.numberOfUpdatedItems());
	CPPUNIT_ASSERT(s1.numberOfDeletedItems() == s2.numberOfDeletedItems());
}
