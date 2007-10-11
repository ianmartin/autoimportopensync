#ifndef SMCSYNCSOURCETEST_H_
#define SMCSYNCSOURCETEST_H_

#include <cppunit/extensions/HelperMacros.h>

#include "SmcSyncSource.h"

class SmcSyncSourceTest : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(SmcSyncSourceTest);
	CPPUNIT_TEST(testConstructor);
	CPPUNIT_TEST(testCopyConstructor);
	CPPUNIT_TEST(testOperatorAssignment);
	CPPUNIT_TEST_SUITE_END();
	
public:
	
	void testConstructor();
	void testCopyConstructor();
	void testOperatorAssignment();
};

#endif /*SMCSYNCSOURCETEST_H_*/
