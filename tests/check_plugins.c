#include <check.h>
#include <opensync/opensync.h>
#include <stdlib.h>

START_TEST (plugin_create)
{
  OSyncPluginInfo *plugin = osync_plugin_new();
  fail_unless(plugin != NULL, "plugin == NULL on creation");
}
END_TEST