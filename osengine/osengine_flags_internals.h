
/*! @brief The functions that can be called on a plugin */
struct OSyncFlag {
	/** If this flag is raised */
	osync_bool is_set;
	/** If this flag is currently changing its value */
	osync_bool is_changing;
	/** The default value of the flag if no children are conencted */
	osync_bool default_val;
	/** The parent flag*/
	OSyncFlag *comb_flag;
	/** The cached number of unset child flags */
	unsigned int num_not_set;
	/** The cached number of set child flags */
	unsigned int num_set;
	/** If this flag is a combined flag */
	osync_bool is_comb;
	/** The function to be called when the value changes from neq to pos */
	OSyncFlagTriggerFunc pos_trigger_func;
	/** To first value to be passed to the pos triger function */
	void *pos_user_data1;
	/** To second value to be passed to the pos triger function  */
	void *pos_user_data2;
	/** The function to be called when the value changes from pos to neq */
	OSyncFlagTriggerFunc neg_trigger_func;
	/** To first value to be passed to the neq triger function */
	void *neg_user_data1;
	/** To second value to be passed to the neq triger function */
	void *neg_user_data2;
	/** Should the combined flag be a OR or a AND flag? */
	osync_bool is_any;
};

OSyncFlag *osync_flag_new(OSyncFlag *parent);
OSyncFlag *osync_comb_flag_new(osync_bool any, osync_bool default_val);
void osync_flag_set_pos_trigger(OSyncFlag *flag, OSyncFlagTriggerFunc func, void *data1, void *data2);
void osync_flag_set_neg_trigger(OSyncFlag *flag, OSyncFlagTriggerFunc func, void *data1, void *data2);
void osync_flag_calculate_comb(OSyncFlag *flag);
osync_bool osync_flag_is_set(OSyncFlag *flag);
osync_bool osync_flag_is_not_set(OSyncFlag *flag);
void osync_comb_flag_update(OSyncFlag *combflag, OSyncFlag *flag, osync_bool prev_state);
void osync_flag_changing(OSyncFlag *flag);
void osync_flag_cancel(OSyncFlag *flag);
void osync_flag_unset(OSyncFlag *flag);
void osync_flag_set(OSyncFlag *flag);
void osync_flag_calc_trigger(OSyncFlag *flag, osync_bool oldstate);
void osync_change_flags_detach(OSyncChange *change);
osync_bool osync_flag_get_state(OSyncFlag *flag);
void osync_flag_free(OSyncFlag *flag);
void osync_flag_set_state(OSyncFlag *flag, osync_bool state);
void osync_flag_attach(OSyncFlag *flag, OSyncFlag *target);
void osync_flag_detach(OSyncFlag *flag);
