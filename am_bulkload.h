/* am_bulkload.h */
#ifndef AM_BULKLOAD_H
#define AM_BULKLOAD_H

#include "student_data.h"

/*
 * Performs a bottom-up B+ tree build (bulk-load) for the given index.
 * Assumes the 'sortedEntries' array is already sorted by the key (roll_no).
 *
 * PARAMETERS:
 * fileDesc: File descriptor for the AM index file (opened with PF_OpenFile).
 * sortedEntries: A pointer to an array of (key, rid) pairs, pre-sorted by key.
 * numEntries: The total number of entries in the array.
 * attrType: The type of the key ('i', 'c', 'f').
 * attrLength: The length of the key.
 *
 * RETURNS:
 * AME_OK on success, or an AM/PF error code on failure.
 */
int AM_BulkLoad(int fileDesc, StudentIndexEntry *sortedEntries, int numEntries, 
                char attrType, int attrLength);

#endif /* AM_BULKLOAD_H */