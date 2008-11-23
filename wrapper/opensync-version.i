%inline %{
	static PyObject *version_load_from_default_descriptions() {
		Error *err = NULL;
		OSyncList *list = osync_version_load_from_default_descriptions(&err);
		if (!list) {
			if (!raise_exception_on_error(err))
				wrapper_exception("osync_version_load_from_descriptions failed but did not set error code");
			return NULL;
		}
		return osynclist_to_pylist(list, SWIGTYPE_p_Version);
	}
%}

typedef struct {} Version;
%extend Version {
	Version() {
		Error *err = NULL;
		Version *version = osync_version_new(&err);
		if (raise_exception_on_error(err))
			return NULL;
		else
			return version;
	}

	~Version() {
		osync_version_unref(self);
	}

	char *get_plugin() {
		return osync_version_get_plugin(self);
	}

	void set_plugin(char *plugin) {
		osync_version_set_plugin(self, plugin);
	}

	char *get_priority() {
		return osync_version_get_priority(self);
	}

	void set_priority(char *priority) {
		osync_version_set_priority(self, priority);
	}

	char *get_vendor() {
		return osync_version_get_vendor(self);
	}

	void set_vendor(char *vendor) {
		osync_version_set_vendor(self, vendor);
	}

	char *get_modelversion() {
		return osync_version_get_modelversion(self);
	}

	void set_modelversion(char *modelversion) {
		osync_version_set_modelversion(self, modelversion);
	}

	char *get_firmwareversion() {
		return osync_version_get_firmwareversion(self);
	}

	void set_firmwareversion(char *firmwareversion) {
		osync_version_set_firmwareversion(self, firmwareversion);
	}

	char *get_softwareversion() {
		return osync_version_get_softwareversion(self);
	}

	void set_softwareversion(char *softwareversion) {
		osync_version_set_softwareversion(self, softwareversion);
	}

	char *get_hardwareversion() {
		return osync_version_get_hardwareversion(self);
	}

	void set_hardwareversion(char *hardwareversion) {
		osync_version_set_hardwareversion(self, hardwareversion);
	}

	char *get_identifier() {
		return osync_version_get_identifier(self);
	}

	void set_identifier(char *identifier) {
		osync_version_set_identifier(self, identifier);
	}

	int matches(Version *pattern) {
		Error *err = NULL;
		int ret = osync_version_matches(pattern, self, &err);
		if (!raise_exception_on_error(err) && ret == -1)
			wrapper_exception("osync_version_matches failed but did not set error code");
		return ret;
	}

	Capabilities *find_capabilities() {
		Error *err = NULL;
		Capabilities *ret = osync_version_find_capabilities(self, &err);
		if (!raise_exception_on_error(err) && !ret)
			wrapper_exception("osync_version_find_capabilities failed but did not set error");
		if (ret)
			osync_capabilities_ref(ret);
		return ret;
	}

%pythoncode %{
	plugin = property(get_plugin, set_plugin)
	priority = property(get_priority, set_priority)
	vendor = property(get_vendor, set_vendor)
	modelversion = property(get_modelversion, set_modelversion)
	firmwareversion = property(get_firmwareversion, set_firmwareversion)
	softwareversion = property(get_softwareversion, set_softwareversion)
	hardwareversion = property(get_hardwareversion, set_hardwareversion)
	identifier = property(get_identifier, set_identifier)
%}
};
