#include "engine.h"
#include "engine_internals.h"

void osync_status_conflict(OSyncEngine *engine, OSyncMapping *mapping)
{
	if (engine->conflict_callback)
		engine->conflict_callback(engine, mapping, engine->conflict_userdata);
}

void osync_status_update_member(OSyncEngine *engine, OSyncClient *client, memberupdatetype type)
{
	if (engine->mebstat_callback) {
		MSyncMemberUpdate update;
		update.type = type;
		update.member = client->member;
		engine->mebstat_callback(&update);
	}
}

void osync_status_update_change(OSyncEngine *engine, OSyncChange *change, changeupdatetype type)
{
	OSyncMapping *mapping = osync_change_get_mapping(change);
	if (engine->changestat_callback) {
		MSyncChangeUpdate update;
		update.type = type;
		update.member_id = osync_member_get_id(osync_change_get_member(change));
		update.change = change;
		if (mapping)
			update.mapping_id = osync_mapping_get_id(mapping);
		engine->changestat_callback(engine, &update, engine->changestat_userdata);
	}
}

void osync_status_update_mapping(OSyncEngine *engine, OSyncMapping *mapping, mappingupdatetype type)
{
	OSyncChange *master;
	if (engine->mapstat_callback) {
		MSyncMappingUpdate update;
		update.type = type;
		update.mapping_id = osync_mapping_get_id(mapping);
		if ((master = osync_mapping_get_masterentry(mapping)))
			update.winner = osync_member_get_id(osync_change_get_member(master));
		engine->mapstat_callback(&update);
	}
}

void osync_status_update_engine(OSyncEngine *engine, engineupdatetype type)
{
	if (engine->engstat_callback) {
		OSyncEngineUpdate update;
		update.type = type;
		engine->engstat_callback(engine, &update, engine->engstat_userdata);
	}
}
