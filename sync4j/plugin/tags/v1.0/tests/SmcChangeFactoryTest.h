#ifndef SMCCHANGEFACTORYTEST_H_
#define SMCCHANGEFACTORYTEST_H_

#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include "SmcChangeFactory.h"

using namespace std;

class SmcChangeFactoryTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE(SmcChangeFactoryTest);
  CPPUNIT_TEST(testGetChange);
  CPPUNIT_TEST_SUITE_END();

public:
	void setUp();
	void tearDown();
	
	void testGetChange();
  
private:
	string* data; 
	string* key;
	string* datatype;
	OSyncEnv* createOSEnvironment();
	void freeOSEnvironment( OSyncEnv* env );
	SyncItem* createItem();
};

#endif /*SMCCHANGEFACTORYTEST_H_*/
