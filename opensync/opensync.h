#ifndef HAVE_OPENSYNC_H
#define HAVE_OPENSYNC_H

/**
 * @defgroup PublicAPI Public APIs
 * @brief Available public APIs
 * 
 */

/**
 * @defgroup OSyncPublic OpenSync Public API
 * @ingroup PublicAPI
 * @brief The public API of opensync
 * 
 * This gives you an insight in the public API of opensync.
 * 
 */
/** @{ */

#include <unistd.h>
#include <sys/types.h>

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

#ifdef __cplusplus
}
#endif

/** @} */

#endif
