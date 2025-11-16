/* blinternal.h - Bulk-Load Layer Internal Structures */

#ifndef BL_INTERNAL_H
#define BL_INTERNAL_H

#include "bl.h"

/* Page types */
#define BL_HEADER_PAGE 0
#define BL_LEAF_PAGE 1
#define BL_INTERNAL_PAGE 2

/* Index file header (stored on page 0) */
typedef struct BL_HeaderPage {
    int rootPage;       /* Page number of root */
    int firstLeaf;      /* Page number of first leaf */
    int numLevels;      /* Height of the tree */
    int numRecords;     /* Total records in index */
    int keyType;        /* Always 'i' for int in this implementation */
    int keyLength;      /* Always sizeof(int) */
} BL_HeaderPage;

/* Leaf page header */
typedef struct BL_LeafHeader {
    int pageType;       /* BL_LEAF_PAGE */
    int numKeys;        /* Number of keys in this page */
    int prevPage;       /* Previous leaf page (for backward scan) */
    int nextPage;       /* Next leaf page (for forward scan) */
} BL_LeafHeader;

/* Internal (non-leaf) page header */
typedef struct BL_InternalHeader {
    int pageType;       /* BL_INTERNAL_PAGE */
    int numKeys;        /* Number of keys in this page */
    int level;          /* Level in the tree (0 = just above leaves) */
} BL_InternalHeader;

/* Calculate max entries per page */
#define BL_MAX_LEAF_ENTRIES \
    ((PF_PAGE_SIZE - sizeof(BL_LeafHeader)) / (sizeof(int) * 2))

#define BL_MAX_INTERNAL_ENTRIES \
    ((PF_PAGE_SIZE - sizeof(BL_InternalHeader)) / (sizeof(int) * 2))

/* Helper macros to access page data */
#define BL_LEAF_KEYS(pageBuf) \
    ((int*)((char*)(pageBuf) + sizeof(BL_LeafHeader)))

#define BL_LEAF_VALUES(pageBuf) \
    ((int*)((char*)(pageBuf) + sizeof(BL_LeafHeader) + \
            BL_MAX_LEAF_ENTRIES * sizeof(int)))

#define BL_INTERNAL_KEYS(pageBuf) \
    ((int*)((char*)(pageBuf) + sizeof(BL_InternalHeader)))

#define BL_INTERNAL_PTRS(pageBuf) \
    ((int*)((char*)(pageBuf) + sizeof(BL_InternalHeader) + \
            BL_MAX_INTERNAL_ENTRIES * sizeof(int)))

#endif /* BL_INTERNAL_H */