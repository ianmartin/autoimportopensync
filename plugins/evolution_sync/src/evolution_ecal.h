#ifndef ECAL_H
#define ECAL_H

void evo_calendar_setup(OSyncPluginInfo *info);
osync_bool evo_calendar_open(evo_environment *env);
osync_bool evo_calendar_get_all (OSyncContext *ctx);
osync_bool evo_calendar_get_changes(OSyncContext *ctx);

#endif /*  ECAL_H */
