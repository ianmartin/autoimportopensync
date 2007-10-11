#ifndef SYNC4JCLIENTTEST_H_
#define SYNC4JCLIENTTEST_H_

#include <cppunit/extensions/HelperMacros.h>
//#include <spds/SyncManagerConfig.h>

#include "Sync4jClient.h"

class Sync4jClientTest : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(Sync4jClientTest);
	CPPUNIT_TEST(testCreateSyncSource);
	CPPUNIT_TEST_SUITE_END();
	
public:
	
	void testCreateSyncSource();
};

#endif /*SYNC4JCLIENTTEST_H_*/
