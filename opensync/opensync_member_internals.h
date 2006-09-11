
/*! @brief A member of a group which represent a single device */
struct OSyncMember {
#ifndef DOXYGEN_SHOULD_SKIP_THIS
	long long int id;
	char *configdir;
	char *configdata;
	int configsize;
	OSyncPlugin *plugin;
	OSyncMemberFunctions *memberfunctions;
	OSyncGroup *group;
	
	void *enginedata;
	void *plugindata;
	
	/* List of sinks, by format
	 *
	 * Note: only use this field after calling osync_member_require_sink_info()
	 *
	 * @todo Add osync_member_get_format_sinks() function
	 * @todo Review users of format_sinks to check if they may
	 *       possibly fail silently if sink information isn't available
	 */
	GList *format_sinks;

	/* List of sinks, by objtype
	 *
	 * Note: only use this field using osync_member_get_objtype_sinks(), or
	 *       after calling osync_member_require_sink_info()
	 */
	GList *objtype_sinks;


	char *pluginname;
	char *name;
	
	//For the filters
	GList *accepted_objtypes;
	GList *filters;

	char *extension;
	
	void *loop;
#endif
};

OSyncObjTypeSink *osync_member_find_objtype_sink(OSyncMember *member, const char *objtypestr);
void osync_member_select_format(OSyncMember *member, OSyncObjTypeSink *objsink);
osync_bool osync_member_instance_default_plugin(OSyncMember *member, OSyncError **error);
OSyncObjFormatSink *osync_member_make_random_data(OSyncMember *member, OSyncChange *change, const char *objtypename);
osync_bool osync_member_get_objtype_sinks(OSyncMember *member, GList **list_ptr, OSyncError **error);
osync_bool osync_member_require_sink_info(OSyncMember *member, OSyncError **error);

void osync_member_write_sink_info(OSyncMember *member, OSyncMessage *message);
void osync_member_read_sink_info(OSyncMember *member, OSyncMessage *message);
void osync_member_read_sink_info_full(OSyncMember *member, OSyncMessage *message);

