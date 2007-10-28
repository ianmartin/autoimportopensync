#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <opensync/opensync.h>

#include "config.h"

char *setup_testbed(char *fkt_name);
void destroy_testbed(char *path);
// create_case() with timeout of 30seconds (default)
void create_case(Suite *s, const char *name, TFun function);
// create_case_timeout() allow to specific a specific timeout - intended for breaking testcases which needs longer then 30seconds (default)
void create_case_timeout(Suite *s, const char *name, TFun function, int timeout);

