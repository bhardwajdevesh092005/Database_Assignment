/* obj3_eval.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pf.h"       /* PF Layer */
#include "sp.h"       /* SP Layer */
#include "am.h"       /* AM Layer */
#include "student_data.h" /* Student data helpers */
#include "am_bulkload.h"  /* Our new bulk-load function */

/* Externs from the PF layer for stats */
extern int logical_reads;
extern int physical_reads;
extern int physical_writes;

/*
 * Runs and times a specific evaluation function.
 * Prints the name, time, and PF stats.
 */
static void time_and_stats(const char* method_name, void (*eval_func)(void)) {
    clock_t start, end;
    double cpu_time_used;

    printf("\n======================================================\n");
    printf("  Running Evaluation: %s\n", method_name);
    printf("======================================================\n");

    /* Reset stats and start timer */
    PF_InitStats();
    start = clock();
    
    (*eval_func)(); /* Execute the method */
    
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    
    /* Print results */
    printf("\n--- Results for: %s ---\n", method_name);
    printf("  Query Completion Time: %f seconds\n", cpu_time_used);
    printf("  Page Access Statistics:\n");
    PF_PrintStats();
    printf("------------------------------------------------------\n");
}

/*
 * METHOD 1: Build index on an existing, unsorted file.
 * 1. Populate Student file (unsorted).
 * 2. Create empty Index file.
 * 3. Scan Student file and call AM_InsertEntry() for each record.
 */
void eval_method_1() {
    int sp_fd, am_fd;
    StudentIndexEntry* entries;
    char index_fname[AM_MAX_FNAME_LENGTH];
    sprintf(index_fname, "%s.%d", STUDENT_FILE, ROLLNO_INDEX);

    /* 1. Create and populate Student file (UNSORTED) */
    SP_CreateFile(STUDENT_FILE);
    sp_fd = SP_OpenFile(STUDENT_FILE);
    entries = populate_student_file(sp_fd, NUM_STUDENT_RECORDS, 0); /* 0 = unsorted */
    SP_CloseFile(sp_fd);
    if (entries == NULL) return;

    /* 2. Create index file */
    AM_CreateIndex(STUDENT_FILE, ROLLNO_INDEX, INT_TYPE, sizeof(int));
    am_fd = PF_OpenFile(index_fname); 
    if (am_fd < 0) { PF_PrintError("Method 1: Open index failed"); free(entries); return; }

    /* 3. Populate index from existing file (unsorted entries) */
    printf("Building index for Method 1 (from existing unsorted data)...\n");
    for (int i = 0; i < NUM_STUDENT_RECORDS; i++) {
        if (AM_InsertEntry(am_fd, INT_TYPE, sizeof(int), (char*)&entries[i].roll_no, entries[i].packed_rid) != AME_OK) {
            fprintf(stderr, "Method 1: AM_InsertEntry failed for roll_no %d\n", entries[i].roll_no);
        }
    }

    /* Cleanup */
    PF_CloseFile(am_fd);
    free(entries);
    SP_DestroyFile(STUDENT_FILE);
    AM_DestroyIndex(STUDENT_FILE, ROLLNO_INDEX);
}

/*
 * METHOD 2: Build index incrementally with an empty file.
 * 1. Create empty Student file.
 * 2. Create empty Index file.
 * 3. Loop:
 * a. SP_InsertRec() into Student file.
 * b. AM_InsertEntry() into Index file.
 */
void eval_method_2() {
    int sp_fd, am_fd;
    StudentRecord rec;
    SP_RID rid;
    char index_fname[AM_MAX_FNAME_LENGTH];
    sprintf(index_fname, "%s.%d", STUDENT_FILE, ROLLNO_INDEX);

    /* 1. Create empty files */
    SP_CreateFile(STUDENT_FILE);
    AM_CreateIndex(STUDENT_FILE, ROLLNO_INDEX, INT_TYPE, sizeof(int));
    
    sp_fd = SP_OpenFile(STUDENT_FILE);
    am_fd = PF_OpenFile(index_fname);
    if (sp_fd < 0 || am_fd < 0) { PF_PrintError("Method 2: Open files failed"); return; }

    /* 2. Populate file and index incrementally (UNSORTED) */
    printf("Building index for Method 2 (incrementally, unsorted)...\n");
    for (int i = 0; i < NUM_STUDENT_RECORDS; i++) {
        rec.roll_no = 10000 + (rand() % (NUM_STUDENT_RECORDS * 2));
        sprintf(rec.name, "Student_%d", rec.roll_no);
        sprintf(rec.dept, "CS");

        /* a. Insert into data file */
        if (SP_InsertRec(sp_fd, (char*)&rec, sizeof(StudentRecord), &rid) != SP_OK) {
            fprintf(stderr, "Method 2: SP_InsertRec failed\n");
            break;
        }
        
        /* b. Insert into index */
        int packed_rid = pack_rid(rid);
        if (AM_InsertEntry(am_fd, INT_TYPE, sizeof(int), (char*)&rec.roll_no, packed_rid) != AME_OK) {
            fprintf(stderr, "Method 2: AM_InsertEntry failed for roll_no %d\n", rec.roll_no);
        }
    }

    /* Cleanup */
    PF_CloseFile(am_fd);
    SP_CloseFile(sp_fd);
    SP_DestroyFile(STUDENT_FILE);
    AM_DestroyIndex(STUDENT_FILE, ROLLNO_INDEX);
}

/*
 * METHOD 3: Build index using bulk-loading from a sorted file.
 * 1. Populate Student file (SORTED).
 * 2. Create empty Index file.
 * 3. Call AM_BulkLoad() to build the index from the sorted data.
 */
void eval_method_3() {
    int sp_fd, am_fd;
    StudentIndexEntry* entries;
    char index_fname[AM_MAX_FNAME_LENGTH];
    sprintf(index_fname, "%s.%d", STUDENT_FILE, ROLLNO_INDEX);

    /* 1. Create and populate Student file (SORTED) */
    SP_CreateFile(STUDENT_FILE);
    sp_fd = SP_OpenFile(STUDENT_FILE);
    entries = populate_student_file(sp_fd, NUM_STUDENT_RECORDS, 1); /* 1 = sorted */
    SP_CloseFile(sp_fd);
    if (entries == NULL) return;
    
    /* (Data is already sorted by populate_student_file, no qsort needed) */

    /* 2. Create index file */
    AM_CreateIndex(STUDENT_FILE, ROLLNO_INDEX, INT_TYPE, sizeof(int));
    am_fd = PF_OpenFile(index_fname);
    if (am_fd < 0) { PF_PrintError("Method 3: Open index failed"); free(entries); return; }

    /* 3. Run Bulk-Load */
    printf("Building index for Method 3 (Bulk-Loading from sorted data)...\n");
    if (AM_BulkLoad(am_fd, entries, NUM_STUDENT_RECORDS, INT_TYPE, sizeof(int)) != AME_OK) {
        fprintf(stderr, "Method 3: AM_BulkLoad failed\n");
    }
    
    /* Cleanup */
    PF_CloseFile(am_fd);
    free(entries);
    SP_DestroyFile(STUDENT_FILE);
    AM_DestroyIndex(STUDENT_FILE, ROLLNO_INDEX);
}

/* --- Main Evaluation Function --- */
int main() {
    srand(time(NULL));
    
    /* Initialize all layers */
    PF_Init();
    SP_Init(); /* Just calls PF_Init() again, which is harmless */

    printf("Objective 3 Evaluation: Index Construction Performance\n");
    printf("========================================================\n");
    printf("Database Size: %d records\n", NUM_STUDENT_RECORDS);
    printf("Buffer Pool Size: %d pages\n", PF_MAX_BUFS);
    printf("Page Size: %d bytes\n", PF_PAGE_SIZE);
    
    /* --- Run Evaluations --- */
    
    /* Note: Methods 1 and 2 use unsorted data, simulating random inserts. */
    /* Method 3 uses sorted data as required for bulk-loading. */
    
    time_and_stats("Method 1: Indexing Existing (Unsorted) File", eval_method_1);
    time_and_stats("Method 2: Incremental Indexing (Unsorted)", eval_method_2);
    time_and_stats("Method 3: Bulk-Loading (Sorted)", eval_method_3);
    
    printf("\nEvaluation complete.\n");
    return 0;
}