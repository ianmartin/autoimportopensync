#ifndef EBOOK_H
#define EBOOK_H

void evo2_addrbook_setup(OSyncPluginInfo *info);
osync_bool evo2_addrbook_open(evo_environment *env, OSyncError **error);
void evo2_addrbook_get_changes(OSyncContext *ctx);

#endif /*  EBOOK_H */
