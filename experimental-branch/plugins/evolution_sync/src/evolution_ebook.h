#ifndef EBOOK_H
#define EBOOK_H

void *evo_addrbook_open(OSyncContext *ctx);
void evo_addrbook_get_changes(OSyncContext *ctx);
osync_bool evo_addrbook_get_all(OSyncContext *ctx);
void evo_addrbook_setup(OSyncPluginInfo *info);

#endif /*  EBOOK_H */
