#ifndef XMLFORMAT_EVENT_H_
#define XMLFORMAT_EVENT_H_

#include <opensync/opensync.h>
#include <opensync/opensync-merger.h>
#include <opensync/opensync-serializer.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-time.h>

#include "vformat.h"
#include "xmlformat.h"

OSyncConvCmpResult compare_event(const char *leftdata, unsigned int leftsize, const char *rightdata, unsigned int rightsize);
void create_event(char **data, unsigned int *size);
time_t get_revision(const char *data, unsigned int size, OSyncError **error);

#endif //XMLFORMAT_EVENT_H_
