#include <opensync/opensync.h>

int get_info(OSyncPluginInfo *info)
{
    info->version = 1;
    info->name = "syncml";
    /*FIXME: i18n */
    info->description = "SyncML plugin";
}
