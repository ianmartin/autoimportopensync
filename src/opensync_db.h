DB *osync_db_open(char *filename, char *dbname, int type, DB_ENV *dbenv);
osync_bool osync_db_put(DB *dbp, void *key, int keysize, void *data, int datasize);
osync_bool osync_db_del(DB *dbp, void *key, int keysize);
void osync_db_close(DB *dbp);
DBC *osync_db_cursor_new(DB *dbp);
osync_bool osync_db_cursor_next(DBC *dbcp, void **key, void **data);
osync_bool osync_db_get(DB *dbp, void *key, int keysize, void **target);
void osync_db_cursor_close(DBC *dbcp);
osync_bool osync_db_cursor_next_sec(DBC *dbcp, void **pkey, void **skey, void **data);
DB *osync_db_open_secondary(DB *firstdb, char *filename, char *dbname, int (*callback)(DB *, const DBT *, const DBT *, DBT *), DB_ENV *dbenv);
//u_long osync_db_create_unique_id(DB *dbp);
void osync_db_sync(DB *dbp);
osync_bool osync_db_put_dbt(DB *dbp, DBT *key, DBT *data);
DB_ENV *osync_db_setup(char *configdir, FILE *errfp);
void osync_db_empty(DB *db);
