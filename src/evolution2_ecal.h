#ifndef ECAL_H
#define ECAL_H

void evo2_calendar_setup(OSyncPluginInfo *info);
osync_bool evo2_calendar_open(evo_environment *env, OSyncError **error);
void evo2_calendar_get_changes(OSyncContext *ctx);

#endif /*  ECAL_H */
