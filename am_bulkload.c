/* am_bulkload.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pf.h"
#include "am.h"
#include "am_bulkload.h"

/* Helper to initialize a new leaf page header */
static void init_leaf_page(char* pageBuf, int attrLength, int maxKeys) {
    AM_LEAFHEADER header;
    header.pageType = 'l';
    header.nextLeafPage = AM_NULL_PAGE;
    header.recIdPtr = PF_PAGE_SIZE;
    header.keyPtr = AM_sl;
    header.freeListPtr = AM_NULL;
    header.numinfreeList = 0;
    header.attrLength = (short)attrLength;
    header.numKeys = 0;
    /* maxKeys from AM_CreateIndex is for internal nodes, leaf is different */
    /* A reasonable estimate: */
    int recSize = attrLength + AM_ss;
    int recIdSize = AM_si + AM_ss;
    header.maxKeys = (PF_PAGE_SIZE - AM_sl - recIdSize) / (recSize + recIdSize);
    
    memcpy(pageBuf, &header, AM_sl);
}

/* Helper to initialize a new internal page header */
static void init_int_page(char* pageBuf, int attrLength, int maxKeys) {
    AM_INTHEADER header;
    header.pageType = 'i';
    header.numKeys = 0;
    header.maxKeys = (short)maxKeys;
    header.attrLength = (short)attrLength;
    memcpy(pageBuf, &header, AM_sint);
}

/*
 * A simplified insert function for bulk-loading.
 * It just appends a (key, rid) pair to a leaf page.
 * Assumes the page is not full and the key is new.
 */
static void simple_leaf_insert(char* pageBuf, AM_LEAFHEADER* header, char* key, int packed_rid) {
    int recSize = header->attrLength + AM_ss;
    int recIdSize = AM_si + AM_ss;
    short null_ptr = AM_NULL;

    /* 1. Add key to the end of the key list */
    int keyOffset = header->keyPtr;
    memcpy(pageBuf + keyOffset, key, header->attrLength);
    
    /* 2. Add RecID to the start of the data area */
    header->recIdPtr -= recIdSize;
    short recIdOffset = header->recIdPtr;
    
    /* Copy the packed int recId and a NULL next-pointer */
    memcpy(pageBuf + recIdOffset, &packed_rid, AM_si); 
    memcpy(pageBuf + recIdOffset + AM_si, &null_ptr, AM_ss);

    /* 3. Point the key to the new RecID entry */
    memcpy(pageBuf + keyOffset + header->attrLength, &recIdOffset, AM_ss);

    /* 4. Update header pointers */
    header->numKeys++;
    header->keyPtr += recSize;
}

/*
 * Recursive function to build the internal levels of the B+ tree from bottom up.
 * Returns the page number of the root for this sub-tree.
 */
static int build_internal_levels(int fileDesc, char attrType, int attrLength, int maxIntKeys,
                                 int *childPages, int numChildren, 
                                 StudentIndexEntry *levelKeys) {
    
    /* Base case: If all children fit in a single root node, create it and return. */
    if (numChildren <= maxIntKeys + 1) {
        int rootPageNum;
        char* rootPageBuf;
        
        /* If this is the *only* level (all leaves), the root is page 0 (which is a leaf) */
        /* This function should only be called if numChildren > 1 */
        /* We will overwrite Page 0 to become the new root */
        if (PF_GetThisPage(fileDesc, AM_RootPageNum, &rootPageBuf) != PFE_OK) return AME_PF;

        init_int_page(rootPageBuf, attrLength, maxIntKeys);
        AM_INTHEADER* header = (AM_INTHEADER*)rootPageBuf;
        
        int recSize = attrLength + AM_si;
        int keyOffset = AM_sint + AM_si;

        /* Write the first child pointer */
        memcpy(rootPageBuf + AM_sint, &childPages[0], AM_si);

        /* Write (key, pointer) pairs */
        for (int i = 0; i < numChildren - 1; i++) {
            memcpy(rootPageBuf + keyOffset, &levelKeys[i].roll_no, attrLength);
            memcpy(rootPageBuf + keyOffset + attrLength, &childPages[i + 1], AM_si);
            header->numKeys++;
            keyOffset += recSize;
        }
        
        PF_UnfixPage(fileDesc, AM_RootPageNum, TRUE);
        return AM_RootPageNum;
    }

    /* Recursive case: More than one internal page is needed at this level */
    int numParentPages = 0;
    int* parentChildPages = malloc(numChildren * sizeof(int)); // Over-alloc
    StudentIndexEntry* parentLevelKeys = malloc(numChildren * sizeof(StudentIndexEntry)); // Over-alloc
    
    int childIdx = 0;
    
    while (childIdx < numChildren) {
        int parentPageNum;
        char* parentPageBuf;
        
        if (PF_AllocPage(fileDesc, &parentPageNum, &parentPageBuf) != PFE_OK) return AME_PF;
        init_int_page(parentPageBuf, attrLength, maxIntKeys);
        AM_INTHEADER* header = (AM_INTHEADER*)parentPageBuf;

        /* Write first child pointer */
        memcpy(parentPageBuf + AM_sint, &childPages[childIdx], AM_si);
        
        /* Save key for *next* level up (i.e., the first key we are about to insert) */
        if (numParentPages > 0) {
            parentLevelKeys[numParentPages - 1] = levelKeys[childIdx];
        }
        
        int recSize = attrLength + AM_si;
        int keyOffset = AM_sint + AM_si;

        /* Fill the internal page */
        while(header->numKeys < header->maxKeys && (childIdx + 1) < numChildren) {
            memcpy(parentPageBuf + keyOffset, &levelKeys[childIdx].roll_no, attrLength);
            memcpy(parentPageBuf + keyOffset + attrLength, &childPages[childIdx + 1], AM_si);
            
            header->numKeys++;
            keyOffset += recSize;
            childIdx++;
        }
        childIdx++; // Move to the next child for the next parent page
        
        parentChildPages[numParentPages] = parentPageNum;
        numParentPages++;
        PF_UnfixPage(fileDesc, parentPageNum, TRUE);
    }
    
    /* Recursively build the next level up */
    int newRoot = build_internal_levels(fileDesc, attrType, attrLength, maxIntKeys,
                                    parentChildPages, numParentPages, parentLevelKeys);

    free(parentChildPages);
    free(parentLevelKeys);
    return newRoot;
}

/* Main bulk-loading function */
int AM_BulkLoad(int fileDesc, StudentIndexEntry *sortedEntries, int numEntries, 
                char attrType, int attrLength) {

    char* pageBuf;
    int pageNum;
    AM_LEAFHEADER* header;
    int maxIntKeys;
    int recSize = attrLength + AM_ss;
    int recIdSize = AM_si + AM_ss;
    int maxLeafKeys;

    /* Get maxIntKeys from the root page (created by AM_CreateIndex) */
    if (PF_GetThisPage(fileDesc, AM_RootPageNum, &pageBuf) != PFE_OK) return AME_PF;
    header = (AM_LEAFHEADER*)pageBuf;
    maxIntKeys = header->maxKeys; 
    
    /* Estimate max leaf keys based on AM_CreateIndex logic */
    maxLeafKeys = (PF_PAGE_SIZE - AM_sl - recIdSize) / (recSize + recIdSize); 
    if (maxLeafKeys > (PF_PAGE_SIZE - AM_sl) / recSize) maxLeafKeys = (PF_PAGE_SIZE - AM_sl) / recSize;

    /* Initialize first leaf page (which is the root page, page 0) */
    init_leaf_page(pageBuf, attrLength, maxIntKeys); 
    header = (AM_LEAFHEADER*)pageBuf; /* Re-point after init */
    
    int currentLeafPageNum = AM_RootPageNum;
    AM_LeftPageNum = AM_RootPageNum; /* Set the global */

    int numLeafPages = 0;
    /* Arrays to store the first key and page# of each new page, for parent creation */
    StudentIndexEntry* levelKeys = malloc(numEntries * sizeof(StudentIndexEntry));
    int* childPages = malloc(numEntries * sizeof(int));
    
    int i = 0;
    while (i < numEntries) {
        /* Check if current leaf page is full */
        if (header->numKeys >= maxLeafKeys || (header->recIdPtr - header->keyPtr) < (recSize + recIdSize)) {
            
            int newPageNum;
            char* newPageBuf;
            if (PF_AllocPage(fileDesc, &newPageNum, &newPageBuf) != PFE_OK) return AME_PF;
            
            /* Save the first key of this new page for the parent node */
            levelKeys[numLeafPages] = sortedEntries[i];
            childPages[numLeafPages] = currentLeafPageNum;
            numLeafPages++;

            /* Link previous leaf to this new leaf */
            header->nextLeafPage = newPageNum;
            memcpy(pageBuf, header, AM_sl); /* Write header change to old buffer */
            PF_UnfixPage(fileDesc, currentLeafPageNum, TRUE); /* Unfix old page */
            
            /* Init new page */
            init_leaf_page(newPageBuf, attrLength, maxIntKeys);
            header = (AM_LEAFHEADER*)newPageBuf;
            pageBuf = newPageBuf;
            currentLeafPageNum = newPageNum;
        }
        
        /* Insert the entry into the current leaf page */
        simple_leaf_insert(pageBuf, header, (char*)&sortedEntries[i].roll_no, sortedEntries[i].packed_rid);
        i++;
    }
    
    /* Save the last leaf page */
    childPages[numLeafPages] = currentLeafPageNum;
    numLeafPages++;
    PF_UnfixPage(fileDesc, currentLeafPageNum, TRUE);

    /* --- Build Internal Nodes --- */
    if (numLeafPages > 1) {
        int newRootPageNum = build_internal_levels(fileDesc, attrType, attrLength, maxIntKeys,
                                             childPages, numLeafPages, levelKeys);
        
        /* If the returned root is not page 0, copy it to page 0 */
        if (newRootPageNum != AM_RootPageNum) {
            char *rootBuf, *page0Buf;
            if (PF_GetThisPage(fileDesc, newRootPageNum, &rootBuf) != PFE_OK) return AME_PF;
            if (PF_GetThisPage(fileDesc, AM_RootPageNum, &page0Buf) != PFE_OK) return AME_PF;
            
            memcpy(page0Buf, rootBuf, PF_PAGE_SIZE);
            
            PF_UnfixPage(fileDesc, AM_RootPageNum, TRUE);
            PF_UnfixPage(fileDesc, newRootPageNum, FALSE);
            PF_DisposePage(fileDesc, newRootPageNum);
        }
    }
    
    free(levelKeys);
    free(childPages);
    return AME_OK;
}