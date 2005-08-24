//Specify any structs etc here.
#include <rra/syncmgr.h>
#include <rra/timezone.h>



typedef struct ids_list {
	uint32_t changed_count;
	uint32_t unchanged_count;
	uint32_t deleted_count;
	uint32_t *changed_ids;
	uint32_t *unchanged_ids;
	uint32_t *deleted_ids;
	RRA_SyncMgrType *type;
} ids_list;


typedef struct plugin_environment {
	OSyncMember *member;
	//If you need a hashtable:
	OSyncHashTable *hashtable;

	RRA_SyncMgr* syncmgr; //connessione a synce
	RRA_Timezone timezone;
	int last_change_counter;
	int change_counter;
	ids_list* contact_ids;
	ids_list* todo_ids;
	ids_list* cal_ids;
	
} plugin_environment;

