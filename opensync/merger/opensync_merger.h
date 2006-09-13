#ifndef OPENSYNC_MERGER_H_
#define OPENSYNC_MERGER_H_

OSYNC_EXPORT OSyncMerger *osync_merger_new(OSyncCapabilities *capabilities, OSyncError **error);
OSYNC_EXPORT void osync_merger_ref(OSyncMerger *merger);
OSYNC_EXPORT void osync_merger_unref(OSyncMerger *merger);

OSYNC_EXPORT void osync_merger_merge(OSyncMerger *merger, OSyncXMLFormat *xmlformat, OSyncXMLFormat *entire);

#endif /*OPENSYNC_MERGER_H_*/
