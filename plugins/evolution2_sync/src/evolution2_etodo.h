#ifndef ETODO_H
#define ETODO_H

osync_bool evo2_todo_open(evo_environment *env, OSyncError **error);
void evo2_tasks_setup(OSyncPluginInfo *info);
void evo2_todo_get_changes(OSyncContext *ctx);

#endif /*  ETODO_H */
