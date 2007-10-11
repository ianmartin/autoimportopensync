#include "ItemFactoryTest.h"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( ItemFactoryTest );

using namespace std;

void 
ItemFactoryTest::testConstructor()
{
	char* mimetype1 = "text/plain";
	ItemFactory factory( mimetype1 );
	string expected( mimetype1 );
	string actual( factory.getMimeType() );
	CPPUNIT_ASSERT_EQUAL( expected, actual );
	
	char* mimetype2 = NULL;
	ItemFactory factory2( mimetype2 );
	expected.clear();
	actual = factory2.getMimeType();
	CPPUNIT_ASSERT_EQUAL( expected, actual );
}

void
ItemFactoryTest::testGetItem()
{
	//Setup
	string mimetype( "text/plain" );
	string uid( "1234abcd" );
	char* cdata = "aaaabbbbcccc";
	string data( cdata );
	// End setup
	
	// Test change with no uid
	OSyncChange* noUidChange = osync_change_new();
	ItemFactory factory1( mimetype.c_str() );
	SyncItem* item1 = factory1.getItem( noUidChange );
	CPPUNIT_ASSERT( NULL == item1 );
	
	// Test change with no data
	OSyncChange* noDataChange = osync_change_new();
	osync_change_set_uid( noDataChange, uid.c_str() );
	ItemFactory factory2( mimetype.c_str() );
	SyncItem* item2 = factory2.getItem( noDataChange );
	CPPUNIT_ASSERT( -1 == item2->getDataSize() );
	delete item2;
	
	// Test populated change
	OSyncChange* change = osync_change_new();
	osync_change_set_uid( change, uid.c_str() );
	osync_change_set_data( change, cdata, strlen( cdata ), TRUE );
	ItemFactory factory3( mimetype.c_str() );
	SyncItem* item3 = factory3.getItem( change );
	CPPUNIT_ASSERT( strlen( cdata ) * sizeof(char) == item3->getDataSize() );
	string actual( ( char* ) item3->getData() );
	CPPUNIT_ASSERT_EQUAL( data, actual );
	delete item3;
	
	osync_change_free(noUidChange);
	osync_change_free(noDataChange);
	osync_change_free(change);
}
