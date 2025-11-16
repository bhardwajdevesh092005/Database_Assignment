#include "bl.h"
#include "blinternal.h"
#include <stdio.h>
int BL_PrintStats(int fd)
{                                                                                                                                                                                                                                                                                                          
    char *pb;
    BL_HeaderPage *h;
    if (PF_GetThisPage(fd, 0, &pb, FALSE) != PFE_OK)
        return BL_ERROR;
    h = (BL_HeaderPage *)pb;
    printf("\n=== Bulk-Load Index Statistics ===\n");
    printf("Total Records: %d\n", h->numRecords);
    printf("Tree Height: %d levels\n", h->numLevels);
    printf("Root Page: %d\n", h->rootPage);
    printf("First Leaf: %d\n", h->firstLeaf);
    printf("Max Leaf Entries: %d\n", BL_MAX_LEAF_ENTRIES);
    printf("Max Internal Entries: %d\n", BL_MAX_INTERNAL_ENTRIES);
    printf("===================================\n\n");
    PF_UnfixPage(fd, 0, FALSE);
    return BL_OK;
}
