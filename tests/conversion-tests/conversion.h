#include "support.h"

#include <time.h>

#include <opensync/opensync-format.h>
#include <opensync/opensync-support.h>
#include <opensync/opensync-data.h>

void conv(const char *objtype, const char *filename, const char *extension);
void compare(const char *objtype, const char *lfilename, const char *rfilename, OSyncConvCmpResult result);
time_t get_revision(const char *objtype, const char *filename, const char *extension);



