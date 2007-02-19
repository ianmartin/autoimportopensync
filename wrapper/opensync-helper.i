%inline %{
	static bool anchor_compare(const char *anchordb, const char *key, const char *new_anchor) {
		return osync_anchor_compare(anchordb, key, new_anchor);
	}

	static void anchor_update(const char *anchordb, const char *key, const char *new_anchor) {
		osync_anchor_update(anchordb, key, new_anchor);
	}

	static char *anchor_retrieve(const char *anchordb, const char *key) {
		return osync_anchor_retrieve(anchordb, key);
	}
%}


typedef struct {} HashTable;
%extend HashTable {
	HashTable(PyObject *obj) {
		return PyCObject_AsVoidPtr(obj);
	}

	HashTable(const char *path, const char *objtype) {
		Error *err = NULL;
		HashTable *hashtable = osync_hashtable_new(path, objtype, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return hashtable;
	}

	~HashTable() {
		/* FIXME: need to free here, but only if we created it! */
		/* osync_hashtable_free(self); */
	}

	void reset() {
		osync_hashtable_reset(self);
	}

	int num_entries() {
		return osync_hashtable_num_entries(self);
	}

	/* returns a tuple of (uid, hash) strings */
	PyObject *nth_entry(int nth) {
		char *uid, *hash;

		if (!osync_hashtable_nth_entry(self, nth, &uid, &hash)) {
			wrapper_exception("osync_hashtable_nth_entry failed");
			return NULL;
		}

		return Py_BuildValue("(ss)", uid, hash);
	}

	void write(const char *uid, const char *hash) {
		osync_hashtable_write(self, uid, hash);
	}

	void delete(const char *uid) {
		osync_hashtable_delete(self, uid);
	}

	void update_hash(ChangeType type, const char *uid, const char *hash) {
		osync_hashtable_update_hash(self, type, uid, hash);
	}

	void report(const char *uid) {
		osync_hashtable_report(self, uid);
	}

	/* returns a list of deleted UIDs as strings */
	PyObject *get_deleted() {
		char **uids = osync_hashtable_get_deleted(self);
		if (uids == NULL) {
			wrapper_exception("osync_hashtable_get_deleted failed");
			return NULL;
		}

		PyObject *ret = PyList_New(0);
		if (ret != NULL) {
			int i;
			for (i = 0; uids[i] != NULL; i++) {
				PyObject *item = PyString_FromString(uids[i]);
				if (item == NULL || PyList_Append(ret, item) != 0) {
					Py_XDECREF(item);
					ret = NULL;
					break;
				}
				Py_DECREF(item);
			}
		}

		free(uids);
		return ret;
	}

	ChangeType get_changetype(const char *uid, const char *hash) {
		return osync_hashtable_get_changetype(self, uid, hash);
	}

%pythoncode %{
	num_entries = property(num_entries)
%}
}