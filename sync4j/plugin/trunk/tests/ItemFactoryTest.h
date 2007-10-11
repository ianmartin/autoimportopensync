#ifndef ITEMFACTORYTEST_H_
#define ITEMFACTORYTEST_H_

#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include "ItemFactory.h"

class ItemFactoryTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE(ItemFactoryTest);
  CPPUNIT_TEST(testConstructor);
  CPPUNIT_TEST(testGetItem);
  CPPUNIT_TEST_SUITE_END();

public:
  void testConstructor();
  void testGetItem();
};

#endif /*ITEMFACTORYTEST_H_*/
