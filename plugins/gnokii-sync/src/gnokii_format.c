#include <opensync/opensync.h>

#include "config.h"
#include "gnokii_calendar_format.h"
#include "gnokii_contact_format.h"

void get_info(OSyncEnv *env)
{

#ifdef HAVE_EVENT	
	gnokii_calendar_format_get_info(env);
#endif	

#ifdef HAVE_CONTACT	
	gnokii_contact_format_get_info(env);	// EXPERIMENTEL!
#endif	

}
