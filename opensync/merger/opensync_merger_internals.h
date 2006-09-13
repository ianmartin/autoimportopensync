#ifndef OPENSYNC_MERGER_INTERNALS_H_
#define OPENSYNC_MERGER_INTERNALS_H_

/**
 * @brief Represent a Merger object
 * @ingroup OSyncMergerPrivateAPI
 */
struct OSyncMerger {
	/** The reference counter for this object */
	int ref_count;
	/** The pointer to the capabilities object */
	OSyncCapabilities *capabilities;
};

#endif /*OPENSYNC_MERGER_INTERNALS_H_*/
