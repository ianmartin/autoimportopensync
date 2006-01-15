#ifndef HAVE_OPENSYNC_H
#define HAVE_OPENSYNC_H

#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************
 * Defines
 *************************************************************/
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/*! 
 * @ingroup OSyncChange
 * @brief The changetypes of a change object */
typedef enum  {
	/** Unknown changetype */
	CHANGE_UNKNOWN = 0,
	/** Object was added */
	CHANGE_ADDED = 1,
	/** Object is unmodifed */
	CHANGE_UNMODIFIED = 2,
	/** Object is deleted */
	CHANGE_DELETED = 3,
	/** Object has been modified */
	CHANGE_MODIFIED = 4
} OSyncChangeType;

/**************************************************************
 * Structs
 *************************************************************/
typedef struct OSyncError OSyncError;
typedef struct OSyncEnv OSyncEnv;
typedef struct OSyncPlugin OSyncPlugin;
typedef struct OSyncGroup OSyncGroup;
typedef struct OSyncUserInfo OSyncUserInfo;
typedef struct OSyncMember OSyncMember;
typedef struct OSyncChange OSyncChange;
typedef struct OSyncContext OSyncContext;
typedef struct OSyncHashTable OSyncHashTable;
typedef struct OSyncFormatEnv OSyncFormatEnv;
typedef struct OSyncObjType OSyncObjType;
typedef struct OSyncObjFormat OSyncObjFormat;
typedef struct OSyncFormatConverter OSyncFormatConverter;
typedef struct OSyncFormatProperty OSyncFormatProperty;
typedef struct OSyncFilter OSyncFilter;
typedef struct OSyncCustomFilter OSyncCustomFilter;
typedef struct OSyncMessage OSyncMessage;
typedef struct OSyncQueue OSyncQueue;
typedef int osync_bool;

#include "opensync_debug.h"
#include "opensync_env.h"
#include "opensync_plugin.h"
#include "opensync_group.h"
#include "opensync_member.h"
#include "opensync_error.h"
#include "opensync_hashtable.h"
#include "opensync_change.h"
#include "opensync_context.h"
#include "opensync_filter.h"
#include "opensync_convert.h"
#include "opensync_changecmds.h"
#include "opensync_convreg.h"
#include "opensync_anchor.h"
#include "opensync_serializer.h"

#ifdef __cplusplus
}
#endif

#endif
