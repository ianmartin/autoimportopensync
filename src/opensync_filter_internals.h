struct OSyncFilter {
	OSyncGroup *group;
	long long int sourcememberid;
	long long int destmemberid;
	char *sourceobjtype;
	char *destobjtype;
	char *detectobjtype;
	OSyncFilterAction action;
	OSyncFilterFunction hook;
};

OSyncFilter *osync_filter_new(void);
