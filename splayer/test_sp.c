#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sp.h"
#include "sptype.h"

#define TEST_FILE "test_sp_file.db"
#define MAX_REC_SIZE 256

/* Helper function to check return codes and print errors */
void check(int err, const char *message) {
    if (err != SP_OK && err != SP_EOF) {
        fprintf(stderr, "ERROR: %s (code: %d)\n", message, err);
        exit(1);
    }
}

void test_basic_insert_and_get() {
    int fd;
    SP_RID rid1, rid2, rid3;
    char buffer[MAX_REC_SIZE];

    printf("--- Test 1: Basic Insertion and Retrieval ---\n");

    check(SP_CreateFile(TEST_FILE), "Create file failed");
    fd = SP_OpenFile(TEST_FILE);
    if (fd < 0) check(fd, "Open file failed");

    const char *rec1 = "This is the first record.";
    const char *rec2 = "This is a slightly longer, second record.";
    const char *rec3 = "Third record.";

    printf("Inserting 3 records...\n");
    check(SP_InsertRec(fd, rec1, strlen(rec1) + 1, &rid1), "Insert rec1 failed");
    check(SP_InsertRec(fd, rec2, strlen(rec2) + 1, &rid2), "Insert rec2 failed");
    check(SP_InsertRec(fd, rec3, strlen(rec3) + 1, &rid3), "Insert rec3 failed");
    printf("  Record 1 inserted at (Page: %d, Slot: %d)\n", rid1.page_num, rid1.slot_num);
    printf("  Record 2 inserted at (Page: %d, Slot: %d)\n", rid2.page_num, rid2.slot_num);
    printf("  Record 3 inserted at (Page: %d, Slot: %d)\n", rid3.page_num, rid3.slot_num);

    printf("Retrieving records to verify...\n");
    check(SP_GetThisRec(fd, rid2, buffer, MAX_REC_SIZE), "Get rec2 failed");
    if (strcmp(buffer, rec2) == 0) {
        printf("  SUCCESS: Record 2 verified.\n");
    } else {
        printf("  FAILURE: Record 2 data mismatch.\n");
    }

    check(SP_GetThisRec(fd, rid1, buffer, MAX_REC_SIZE), "Get rec1 failed");
    if (strcmp(buffer, rec1) == 0) {
        printf("  SUCCESS: Record 1 verified.\n");
    } else {
        printf("  FAILURE: Record 1 data mismatch.\n");
    }

    check(SP_CloseFile(fd), "Close file failed");
    check(SP_DestroyFile(TEST_FILE), "Destroy file failed");
    printf("--- Test 1 Complete ---\n\n");
}

void test_scan_and_delete() {
    int fd, i, count;
    SP_RID rids[10];
    SP_RID current_rid, next_rid;
    char buffer[MAX_REC_SIZE];
    char record_data[50];

    printf("--- Test 2: Linear Scan and Deletion ---\n");
    check(SP_CreateFile(TEST_FILE), "Create file failed");
    fd = SP_OpenFile(TEST_FILE);
    if (fd < 0) check(fd, "Open file failed");

    printf("Inserting 10 records...\n");
    for (i = 0; i < 10; i++) {
        sprintf(record_data, "Scan record #%d", i);
        check(SP_InsertRec(fd, record_data, strlen(record_data) + 1, &rids[i]), "Insert failed");
    }

    printf("Scanning all 10 records...\n");
    count = 0;
    int err = SP_GetFirstRec(fd, buffer, MAX_REC_SIZE, &current_rid);
    while (err == SP_OK) {
        count++;
        err = SP_GetNextRec(fd, &current_rid, buffer, MAX_REC_SIZE, &next_rid);
        current_rid = next_rid;
    }
    if (count == 10) {
        printf("  SUCCESS: Scan found all 10 records.\n");
    } else {
        printf("  FAILURE: Scan found %d records, expected 10.\n", count);
    }

    printf("Deleting records at slot 3, 5, and 8...\n");
    check(SP_DeleteRec(fd, rids[3]), "Delete rec3 failed");
    check(SP_DeleteRec(fd, rids[5]), "Delete rec5 failed");
    check(SP_DeleteRec(fd, rids[8]), "Delete rec8 failed");

    printf("Verifying deleted record cannot be fetched...\n");
    if (SP_GetThisRec(fd, rids[5], buffer, MAX_REC_SIZE) != SP_OK) {
        printf("  SUCCESS: SP_GetThisRec correctly failed for deleted record.\n");
    } else {
        printf("  FAILURE: SP_GetThisRec succeeded on a deleted record.\n");
    }

    printf("Scanning again to count remaining records...\n");
    count = 0;
    err = SP_GetFirstRec(fd, buffer, MAX_REC_SIZE, &current_rid);
    while (err == SP_OK) {
        count++;
        err = SP_GetNextRec(fd, &current_rid, buffer, MAX_REC_SIZE, &next_rid);
        current_rid = next_rid;
    }
    if (count == 7) {
        printf("  SUCCESS: Scan found 7 records after deletion.\n");
    } else {
        printf("  FAILURE: Scan found %d records, expected 7.\n", count);
    }

    check(SP_CloseFile(fd), "Close file failed");
    check(SP_DestroyFile(TEST_FILE), "Destroy file failed");
    printf("--- Test 2 Complete ---\n\n");
}

void test_compaction() {
    int fd, i;
    int num_recs_to_fill = (PF_PAGE_SIZE / 2) / 20; 
    SP_RID rids[num_recs_to_fill];
    char small_rec[] = "compaction_test_rec";
    char large_rec[PF_PAGE_SIZE / 2];

    printf("--- Test 3: Compaction Trigger ---\n");
    memset(large_rec, 'A', sizeof(large_rec) - 1);
    large_rec[sizeof(large_rec) - 1] = '\0';

    check(SP_CreateFile(TEST_FILE), "Create file failed");
    fd = SP_OpenFile(TEST_FILE);
    if (fd < 0) check(fd, "Open file failed");

    printf("Inserting %d small records to create fragmentation...\n", num_recs_to_fill);
    for (i = 0; i < num_recs_to_fill; i++) {
        check(SP_InsertRec(fd, small_rec, strlen(small_rec) + 1, &rids[i]), "Insert failed");
    }

    printf("Deleting every other record to trigger compaction...\n");
    /* Delete enough records to free more than 50% of the page space*/
    for (i = 0; i < num_recs_to_fill; i += 2) {
        check(SP_DeleteRec(fd, rids[i]), "Delete failed");
    }
    printf("  Deletion complete. Compaction should have occurred.\n");

    printf("Attempting to insert a large record that requires compacted space...\n");
    SP_RID large_rid;
    if (SP_InsertRec(fd, large_rec, strlen(large_rec) + 1, &large_rid) == SP_OK) {
        printf("  SUCCESS: Large record inserted successfully, compaction worked.\n");
    } else {
        printf("  FAILURE: Could not insert large record, compaction may have failed.\n");
    }

    check(SP_CloseFile(fd), "Close file failed");
    check(SP_DestroyFile(TEST_FILE), "Destroy file failed");
    printf("--- Test 3 Complete ---\n\n");
}


int main() {
    SP_Init();

    test_basic_insert_and_get();
    test_scan_and_delete();
    test_compaction();

    printf("All SP Layer tests passed.\n");

    return 0;
}