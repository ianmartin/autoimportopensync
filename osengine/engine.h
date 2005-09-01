#ifndef HAVE_ENGINE_H
#define HAVE_ENGINE_H

#include <opensync/opensync.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************
 * Structs
 *************************************************************/
typedef struct OSyncEngine OSyncEngine;
typedef struct OSyncClient OSyncClient;
typedef struct OSyncMapping OSyncMapping;

/**************************************************************
 * Includes
 *************************************************************/

#include "osengine_status.h"
#include "osengine_engine.h"
#include "osengine_mapping.h"
#include "osengine_debug.h"

#ifdef __cplusplus
}
#endif

#endif
