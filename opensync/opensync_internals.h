
#include <glib.h>
#include <gmodule.h>
#include <string.h>
#include <glib/gprintf.h>
#include <sys/stat.h>

#include "config.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <errno.h>
extern int errno;

#define osync_assert(x, msg) if (!(x)) { fprintf(stderr, "%s:%i:E:%s: %s\n", __FILE__, __LINE__, __FUNCTION__, msg); abort();}
#define segfault_me char **blablabla = NULL; *blablabla = "test";

#define osync_return_if_fail(condition) do {                                            \
  if (!(condition)) {                                                                   \
    osync_debug ("ASSERT", 0, "%i: Assertion failed: \"%s\" in %s:%i:%s", getpid (), #condition, __FILE__, __LINE__, __FUNCTION__);  \
    return;                                                                             \
  } } while (0)

#define osync_return_val_if_fail(condition, val) do {                                   \
  if (!(condition)) {                                                                   \
    return (val);                                                                       \
  } } while (0)

typedef struct OSyncDB OSyncDB;

#include "opensync_user_internals.h"
#include "opensync_change_internals.h"
#include "opensync_env_internals.h"
#include "opensync_error_internals.h"
#include "opensync_db_internals.h"
#include "opensync_format_internals.h"
#include "opensync_member_internals.h"
#include "opensync_group_internals.h"
#include "opensync_plugin_internals.h"
#include "opensync_filter_internals.h"
#include "opensync_context_internals.h"
#include "opensync_hashtable_internals.h"
