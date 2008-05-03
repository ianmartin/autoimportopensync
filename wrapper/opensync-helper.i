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
	HashTable(const char *path, const char *objtype) {
		Error *err = NULL;
		HashTable *hashtable = osync_hashtable_new(path, objtype, &err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return hashtable;
	}

	~HashTable() {
		osync_hashtable_unref(self);
	}

        bool load() {
                Error *err = NULL;
                osync_hashtable_load(self, &err);

                if (raise_exception_on_error(err))
                        return FALSE;

                return TRUE;
        }

        bool save() {
                Error *err = NULL;
                osync_hashtable_save(self, &err);

                if (raise_exception_on_error(err))
                        return FALSE;

                return TRUE;
        }

        bool slowsync() {
                Error *err = NULL;
                osync_hashtable_slowsync(self, &err);

                if (raise_exception_on_error(err))
                        return FALSE;

                return TRUE;
        }

	int num_entries() {
		return osync_hashtable_num_entries(self);
	}

	void update_change(Change *change) {
		osync_hashtable_update_change(self, change);
	}

	/* returns a list of deleted UIDs as strings */
	PyObject *get_deleted() {
		OSyncList *uids = osync_hashtable_get_deleted(self);
		if (uids == NULL) {
			wrapper_exception("osync_hashtable_get_deleted failed");
			return NULL;
		}

		PyObject *ret = PyList_New(0);
		if (ret != NULL) {
                        OSyncList *u;
			for (u = uids; u; u = u->next) {
                                char *uid = u->data;
				PyObject *item = PyString_FromString(uid);
				if (item == NULL || PyList_Append(ret, item) != 0) {
					Py_XDECREF(item);
					ret = NULL;
					break;
				}
				Py_DECREF(item);
			}
		}

		return ret;
	}

	ChangeType get_changetype(Change *change) {
		return osync_hashtable_get_changetype(self, change);
	}

%pythoncode %{
	# extend the SWIG-generated constructor, so that we can setup our list-wrapper classes
	__oldinit = __init__
	def __init__(self, *args):
		self.__oldinit(*args)
		self.entries = _ListWrapper(self.num_entries, self.nth_entry)
%}
}
