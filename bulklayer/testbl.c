/* testbl.c - Test program for Bulk-Load layer */

#include "bl.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TEST_FILE "test_bulk_index.db"
#define NUM_RECORDS 10000

/* Comparison function for qsort */
int compareKeyVal(const void *a, const void *b) {
    return ((BL_KeyVal*)a)->key - ((BL_KeyVal*)b)->key;
}

int main() {
    int i, err;
    int indexFd;
    BL_KeyVal *data;
    clock_t start, end;
    double buildTime, searchTime;
    int searchKey, searchValue;
    BL_ScanDesc scanDesc;
    int key, value;
    int scanCount;
    
    printf("=== Bulk-Load Layer Test ===\n\n");
    
    /* Initialize */
    BL_Init();
    PF_InitStats();
    
    /* Generate random data */
    printf("Generating %d random records...\n", NUM_RECORDS);
    data = (BL_KeyVal*)malloc(NUM_RECORDS * sizeof(BL_KeyVal));
    
    srand(time(NULL));
    for (i = 0; i < NUM_RECORDS; i++) {
        data[i].key = rand() % 100000;
        data[i].value = i;
    }
    
    /* Sort the data */
    printf("Sorting records by key...\n");
    qsort(data, NUM_RECORDS, sizeof(BL_KeyVal), compareKeyVal);
    
    /* Create index file */
    printf("Creating index file...\n");
    err = BL_CreateIndex(TEST_FILE);
    if (err != BL_OK) {
        printf("Failed to create index\n");
        return 1;
    }
    
    /* Open index */
    indexFd = BL_OpenIndex(TEST_FILE);
    if (indexFd < 0) {
        printf("Failed to open index\n");
        return 1;
    }
    
    /* Bulk-load the data */
    printf("Bulk-loading %d sorted records...\n", NUM_RECORDS);
    PF_InitStats();
    start = clock();
    
    err = BL_BulkLoad(indexFd, data, NUM_RECORDS);
    
    end = clock();
    buildTime = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    if (err != BL_OK) {
        printf("Bulk-load failed\n");
        return 1;
    }
    
    printf("Bulk-load completed in %.6f seconds\n", buildTime);
    printf("\n--- Build Statistics ---\n");
    PF_PrintStats();
    
    /* Print index stats */
    BL_PrintStats(indexFd);
    
    /* Test search operations */
    printf("\n--- Testing Search Operations ---\n");
    PF_InitStats();
    start = clock();
    
    for (i = 0; i < 100; i++) {
        searchKey = data[rand() % NUM_RECORDS].key;
        err = BL_Search(indexFd, searchKey, &searchValue);
        if (err != BL_OK) {
            printf("Search failed for key %d\n", searchKey);
        }
    }
    
    end = clock();
    searchTime = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("100 searches completed in %.6f seconds\n", searchTime);
    printf("Average search time: %.6f seconds\n", searchTime / 100);
    printf("\n--- Search Statistics ---\n");
    PF_PrintStats();
    
    /* Test scan operation */
    printf("\n--- Testing Scan Operation ---\n");
    printf("Scanning all records...\n");
    
    err = BL_OpenScan(indexFd, BL_GE, 0, &scanDesc);
    if (err != BL_OK) {
        printf("Failed to open scan\n");
        return 1;
    }
    
    scanCount = 0;
    while (BL_GetNext(&scanDesc, &key, &value) == BL_OK) {
        scanCount++;
        if (scanCount <= 10) {
            printf("  Record %d: key=%d, value=%d\n", scanCount, key, value);
        }
    }
    
    BL_CloseScan(&scanDesc);
    printf("Total records scanned: %d\n", scanCount);
    
    /* Cleanup */
    BL_CloseIndex(indexFd);
    BL_DestroyIndex(TEST_FILE);
    free(data);
    
    printf("\n=== Test Complete ===\n");
    
    return 0;
}
