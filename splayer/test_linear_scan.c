/*
 * test_linear_scan.c
 *
 * Objective:
 *   1. Create a data file using the SP layer.
 *   2. Insert a number of sample records.
 *   3. Perform a full linear scan of the file using SP_GetFirstRec and SP_GetNextRec.
 *   4. Print the retrieved records to verify correctness.
 *   5. Print the PF layer statistics to measure the I/O performance of the scan.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sp.h"
#include "pf.h"

#define DATA_FILE_NAME "linear_scan_test_db"
#define NUM_RECORDS_TO_INSERT 25

int main() {
    int data_fd;
    int i;
    char buffer[100];
    SP_RID rid;

    printf("--- Linear Scan Test ---\n");

    if (SP_CreateFile(DATA_FILE_NAME) < 0) {
        printf("Error: Could not create data file '%s'.\n", DATA_FILE_NAME);
        exit(1);
    }
    data_fd = SP_OpenFile(DATA_FILE_NAME);
    if (data_fd < 0) {
        printf("Error: Could not open data file '%s'.\n", DATA_FILE_NAME);
        exit(1);
    }
    printf("Successfully created and opened data file: %s\n", DATA_FILE_NAME);

    printf("\nInserting %d sample records...\n", NUM_RECORDS_TO_INSERT);
    for (i = 0; i < NUM_RECORDS_TO_INSERT; i++) {
        sprintf(buffer, "This is sample record number %d.", i + 1);
        if (SP_InsertRec(data_fd, buffer, strlen(buffer) + 1, &rid) < 0) {
            printf("Error: Failed to insert record %d.\n", i + 1);
            SP_CloseFile(data_fd);
            exit(1);
        }
    }
    printf("All records inserted successfully.\n");

    printf("\nInitializing performance counters before scan...\n");
    PF_InitStats();

    printf("\n--- Starting Linear Scan ---\n");
    int records_scanned = 0;
    int err;

    err = SP_GetFirstRec(data_fd, buffer, sizeof(buffer), &rid);
    while (err == SP_OK) {
        records_scanned++;
        printf("  > Scanned Record %d (RID: %d, %d): \"%s\"\n", 
               records_scanned, rid.page_num, rid.slot_num, buffer);

        SP_RID next_rid;
        err = SP_GetNextRec(data_fd, &rid, buffer, sizeof(buffer), &next_rid);
        rid = next_rid;
    }

    if (err != SP_EOF) {
        printf("Error: Scan terminated unexpectedly.\n");
    }
    printf("--- Linear Scan Complete ---\n");

    printf("\n--- Scan Performance Report ---\n");
    printf("Total records scanned: %d\n", records_scanned);
    if (records_scanned == NUM_RECORDS_TO_INSERT) {
        printf("Verification: SUCCESS - Scanned record count matches inserted count.\n");
    } else {
        printf("Verification: FAILED - Mismatch in scanned vs. inserted records.\n");
    }

    printf("\n--- Page Access Statistics (during scan) ---\n");
    PF_PrintStats();

    SP_CloseFile(data_fd);

    return 0;
}
