#include <opensync/opensync.h>

#include "gnokii_calendar_format.h"
#include "gnokii_contact_format.h"

void get_info(OSyncEnv *env)
{

	gnokii_calendar_format_get_info(env);
//	gnokii_contact_format_get_info(env);	// EXPERIMENTEL!

}
