/*
 *
 */
extern osync_bool commit_file_change(OSyncContext *ctx, OSyncChange *change);
extern osync_bool file_get_changeinfo(OSyncContext *ctx);
extern void file_connect(OSyncContext *ctx);
extern  bool file_callback (RRA_SyncMgrTypeEvent event, uint32_t type, uint32_t count, uint32_t* ids, void* cookie);
extern void file_sync_done(OSyncContext *ctx);

