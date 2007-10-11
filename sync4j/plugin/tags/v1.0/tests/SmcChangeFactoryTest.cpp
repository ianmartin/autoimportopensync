#include "SmcChangeFactoryTest.h"


CPPUNIT_TEST_SUITE_REGISTRATION( SmcChangeFactoryTest );

void
SmcChangeFactoryTest::setUp() 
{
	data = new string( "BEGIN:VCALENDARaaaaVERSION:bbbbAAAABEGIN:VEVENT1234" );
	key = new string( "aaa" );
	datatype = new string( "text/x-vcalendar" );
}

void 
SmcChangeFactoryTest::tearDown()
{
	delete data;
	delete key;
	delete datatype;
}

void 
SmcChangeFactoryTest::testGetChange()
{
	SyncItem* item = createItem();
	OSyncEnv* env = createOSEnvironment();
	OSyncMember* member = osync_group_nth_member( osync_env_nth_group( env, 0 ), 0 );
	CPPUNIT_ASSERT( 0 != member );
	SmcChangeFactory factory( member );
	OSyncChange* change = factory.getChange( item, CHANGE_ADDED );
	CPPUNIT_ASSERT( 0 != change );
	string actualKey( osync_change_get_uid( change ) );
	CPPUNIT_ASSERT_EQUAL( *key, actualKey );
	string actualData( osync_change_get_data( change ) );
	CPPUNIT_ASSERT_EQUAL( *data, actualData );
		
	delete item;
	osync_change_free( change );
	freeOSEnvironment( env );
}

OSyncEnv*
SmcChangeFactoryTest::createOSEnvironment()
{
	OSyncEnv* env = osync_env_new();
	OSyncGroup* group = osync_group_new( env );
	OSyncMember* member = osync_member_new( group );
	
	return env;
}

void
SmcChangeFactoryTest::freeOSEnvironment( OSyncEnv* env ) 
{
	OSyncGroup* group = osync_env_nth_group( env, 0 );
	osync_group_free( group );
	osync_env_free( env );
}

SyncItem* 
SmcChangeFactoryTest::createItem()
{
	SyncItem* item = new SyncItem( key->c_str() );
		
	long size = strlen( data->c_str() ) * sizeof( char );
	item->setData( (void*) data->c_str(), size );
	item->setDataType( datatype->c_str() );
	item->setState( SYNC_STATE_NEW );
	
	return item;
}
