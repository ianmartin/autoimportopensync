/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */
 
#include <opensync.h>
#include "opensync_internals.h"

void osync_db_errcall_fcn(const char *errpfx, char *msg)
{
	osync_debug("OSGRP", 3, "BDB error %s: %s", errpfx, msg);
}

DB_ENV *osync_db_setup(char *configdir, FILE *errfp)
{
	DB_ENV *dbenv;
	int ret;

    if ((ret = db_env_create(&dbenv, 0)) != 0) {
    	fprintf(errfp, "opensync: %s\n", db_strerror(ret));
    	return (NULL);
    }
    dbenv->set_errfile(dbenv, errfp);
    dbenv->set_errpfx(dbenv, "opensync");
    
    if ((ret = dbenv->set_verbose(dbenv, DB_VERB_RECOVERY, 1)) != 0) {
    	dbenv->err(dbenv, ret, "set_verbose: db");
    	goto err;
    }

    dbenv->set_errcall(dbenv, osync_db_errcall_fcn);

    /*if ((ret = dbenv->set_cachesize(dbenv, 0, 5 * 1024 * 1024, 0)) != 0) {
    	dbenv->err(dbenv, ret, "set_cachesize");
    	goto err;
    }*/
    
    if ((ret = dbenv->set_data_dir(dbenv, "db")) != 0) {
    	dbenv->err(dbenv, ret, "set_data_dir: db");
    	goto err;
    }

    if ((ret = dbenv->set_flags(dbenv, DB_LOG_AUTOREMOVE, 1)) != 0) {
    	dbenv->err(dbenv, ret, "set_flags: db");
    	goto err;
    }

    if ((ret = dbenv->open(dbenv, configdir, DB_CREATE | DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN | DB_RECOVER | DB_PRIVATE | DB_INIT_LOCK, 0)) != 0) {
    	dbenv->err(dbenv, ret, "environment open: %s", configdir);
    	goto err;
    }

    return (dbenv);

    err:
    	(void)dbenv->close(dbenv, 0);
    	return (NULL);
}

void osync_db_tear_down(DB_ENV *dbenv)
{
	dbenv->close(dbenv, 0);
}

DB *osync_db_open(char *filename, char *dbname, int type, DB_ENV *dbenv)
{
	int ret = 0;
	DB *dbp = NULL;
	if ((ret = db_create(&dbp, NULL, 0)) != 0) {
		printf("db_create: %s\n", db_strerror(ret));
		return NULL;
	}

	//Verify
	//FIXME
	if ((ret = dbp->verify(dbp, g_strdup(filename), NULL, NULL, 0)) != 0) {
		if (ret != 2) { //ENOENT FIXME
			printf("%i verify failed for db %s: %s\n", ret, filename, db_strerror(ret));
			return NULL;
		}
	}
	dbp = NULL;

	if ((ret = db_create(&dbp, dbenv, 0)) != 0) {
		printf("db_create: %s\n", db_strerror(ret));
		return NULL;
	}
	
	if ((ret = dbp->set_flags(dbp, DB_RECNUM)) != 0) {
		dbp->err(dbp, ret, "set_flags: DB_RECNUM");
		return NULL;
	}
	
	//FIXME g_strdup
	if ((ret = dbp->open(dbp, NULL, g_strdup(filename), g_strdup(dbname), type, DB_CREATE, 0664)) != 0) {
		printf("opening db %s", filename);
		return NULL;
	}
	return dbp;
}

int stubcallback(DB *dbp, const DBT *dbt1, const DBT *dbt2, DBT *dbt3)
{
	printf("Stubcallback called\n");
	return DB_DONOTINDEX;
}

void osync_db_sync(DB *dbp)
{
	dbp->sync(dbp, 0);
}

DB *osync_db_open_secondary(DB *firstdb, char *filename, char *dbname, int (*callback)(DB *, const DBT *, const DBT *, DBT *), DB_ENV *dbenv)
{
	DB *sdbp = NULL;
	int ret;
	int (*secfunc)(DB *, const DBT *, const DBT *, DBT *);
	
	if ((ret = db_create(&sdbp, dbenv, 0)) != 0) {
		printf("sec_db_create: %s\n", db_strerror(ret));
		return NULL;
	}
	if ((ret = sdbp->set_flags(sdbp, DB_DUP | DB_DUPSORT)) != 0) {
		printf("sec_db_set_flag: %s\n", db_strerror(ret));
		return NULL;
	}
	if ((ret = sdbp->open(sdbp, NULL, filename, dbname, DB_BTREE, DB_CREATE, 0600)) != 0) {
		printf("sec_db_open: %s\n", db_strerror(ret));
		return NULL;
	}

	if (!callback) {
		secfunc = stubcallback;
	} else {
		secfunc = callback;
	}
	if ((ret = firstdb->associate(firstdb, NULL, sdbp, secfunc, 0)) != 0) {
		printf("sec_db_associate: %s\n", db_strerror(ret));
		return NULL;
	}
	return sdbp;
}

osync_bool osync_db_put_dbt(DB *dbp, DBT *key, DBT *data)
{
	int ret;
	if ((ret = dbp->put(dbp, NULL, key, data, 0)) != 0) {
		dbp->err(dbp, ret, "DB->put");
		return FALSE;
	}
	dbp->sync(dbp, 0);
	return TRUE;
}

osync_bool osync_db_put(DB *dbp, void *key, int keysize, void *data, int datasize)
{
	DBT keydbt, datadbt;
	memset(&keydbt, 0, sizeof(keydbt));
	memset(&datadbt, 0, sizeof(datadbt));
	
	keydbt.data = key;
	keydbt.size = keysize;
	datadbt.data = data;
	datadbt.size = datasize;
	
	return osync_db_put_dbt(dbp, &keydbt, &datadbt);
}

osync_bool osync_db_del_dbt(DB *dbp, DBT *key)
{
	int ret;
	
	if ((ret = dbp->del(dbp, NULL, key, 0)) != 0) {
		dbp->err(dbp, ret, "DB->del");
		return FALSE;
	}
	dbp->sync(dbp, 0);
	return TRUE;
}

osync_bool osync_db_del(DB *dbp, void *key, int keysize)
{
	DBT keydbt;
	memset(&keydbt, 0, sizeof(keydbt));
	
	keydbt.data = key;
	keydbt.size = keysize;
	return osync_db_del_dbt(dbp, &keydbt);
}

void osync_db_close(DB *dbp)
{
	dbp->close(dbp, 0);
}

DBC *osync_db_cursor_new(DB *dbp)
{
	DBC *dbcp;
	int ret;
	
	if ((ret = dbp->cursor(dbp, NULL, &dbcp, 0)) != 0) {
    	dbp->err(dbp, ret, "DB->cursor");
    	return NULL;
    }
	return dbcp;
}

osync_bool osync_db_cursor_next_sec(DBC *dbcp, void **pkey, void **skey, void **data)
{
	DBT pkeydbt, skeydbt, datadbt;
	int ret;

    memset(&pkeydbt, 0, sizeof(pkeydbt));
    memset(&skeydbt, 0, sizeof(skeydbt));
    memset(&datadbt, 0, sizeof(datadbt));
    
    if ((ret = dbcp->c_pget(dbcp, &skeydbt, &pkeydbt, &datadbt, DB_NEXT)) == 0) {
    	*pkey = g_malloc0(pkeydbt.size);
    	memcpy(*pkey, pkeydbt.data, pkeydbt.size);
    	*skey = g_malloc0(skeydbt.size);
    	memcpy(*skey, skeydbt.data, skeydbt.size);
    	*data = g_malloc0(datadbt.size);
    	memcpy(*data, datadbt.data, datadbt.size);
    	return TRUE;
    } else {
    	return FALSE;
    }
}
osync_bool osync_db_cursor_next(DBC *dbcp, void **key, void **data)
{
	DBT keydbt, datadbt;
	int ret;

    memset(&keydbt, 0, sizeof(keydbt));
    memset(&datadbt, 0, sizeof(datadbt));
    
    if ((ret = dbcp->c_get(dbcp, &keydbt, &datadbt, DB_NEXT)) == 0) {
    	*key = g_malloc0(keydbt.size);
    	memcpy(*key, keydbt.data, keydbt.size);
    	*data = g_malloc0(datadbt.size);
    	memcpy(*data, datadbt.data, datadbt.size);
    	return TRUE;
    } else {
    	return FALSE;
    }
}

osync_bool osync_db_get(DB *dbp, void *key, int keysize, void **target)
{
	DBT keydbt, datadbt;
	int ret;
	memset(&keydbt, 0, sizeof(keydbt));
	memset(&datadbt, 0, sizeof(datadbt));
	
	keydbt.data = key;
	keydbt.size = keysize;
	
	if ((ret = dbp->get(dbp, NULL, &keydbt, &datadbt, 0)) == 0) {
		*target = g_malloc0(datadbt.size);
		memcpy(*target, datadbt.data, datadbt.size);
		return TRUE;
	} else {
		if (ret != DB_NOTFOUND) {
			dbp->err(dbp, ret, "DB->get");
		}
		return FALSE;
	}
}

void osync_db_empty(DB *db)
{
	u_int32_t deleted = 0;
	db->truncate(db, NULL, &deleted, DB_AUTO_COMMIT);
}

void osync_db_cursor_close(DBC *dbcp)
{
    dbcp->c_close(dbcp);
}
