#ifndef ETODO_H
#define ETODO_H

void evo_tasks_setup(OSyncPluginInfo *info);
osync_bool evo_tasks_open(evo_environment *env);
osync_bool evo_tasks_get_changes(OSyncContext *ctx);
osync_bool evo_tasks_get_all (OSyncContext *ctx);

#endif /*  ETODO_H */
