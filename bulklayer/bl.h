/* bl.h - Bulk-Load Layer Public API */

#ifndef BL_H
#define BL_H

#include "pf.h"

/* Return codes */
#define BL_OK 0
#define BL_ERROR -1
#define BL_EOF -2
#define BL_KEYNOTFOUND -3

/* Comparison operators */
#define BL_EQ 1  /* Equal */
#define BL_LT 2  /* Less than */
#define BL_GT 3  /* Greater than */
#define BL_LE 4  /* Less than or equal */
#define BL_GE 5  /* Greater than or equal */
#define BL_NE 6  /* Not equal */

/* Key-Value pair for bulk loading */
typedef struct BL_KeyVal {
    int key;
    int value;
} BL_KeyVal;

/* Scan descriptor */
typedef struct BL_ScanDesc {
    int indexFd;
    int op;
    int scanKey;
    int currentPage;
    int currentSlot;
    int isOpen;
} BL_ScanDesc;

/* Initialize the BL layer */
void BL_Init(void);

/* Create a new index file */
int BL_CreateIndex(char *fileName);

/* Destroy an index file */
int BL_DestroyIndex(char *fileName);

/* Open an existing index file */
int BL_OpenIndex(char *fileName);

/* Close an index file */
int BL_CloseIndex(int indexFd);

/* Build index from sorted key-value pairs (bottom-up construction) */
int BL_BulkLoad(int indexFd, BL_KeyVal *sortedData, int numRecords);

/* Search for a key and return its value */
int BL_Search(int indexFd, int key, int *value);

/* Open a scan on the index */
int BL_OpenScan(int indexFd, int op, int scanKey, BL_ScanDesc *scanDesc);

/* Get next entry from scan */
int BL_GetNext(BL_ScanDesc *scanDesc, int *key, int *value);

/* Close a scan */
int BL_CloseScan(BL_ScanDesc *scanDesc);

/* Print index statistics */
int BL_PrintStats(int indexFd);

#endif /* BL_H */