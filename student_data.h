/* student_data.h */
#ifndef STUDENT_DATA_H
#define STUDENT_DATA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sptype.h" /* For SP_RID */
#include "sp.h"     /* For SP_InsertRec */

#define STUDENT_FILE "student.db"
#define ROLLNO_INDEX 0
#define NUM_STUDENT_RECORDS 5000 /* Number of records to test with */

/* * A simple student record. 
 * We use a fixed-size struct, but the SP layer will store it
 * as a variable-length record (sizeof(StudentRecord)).
 */
typedef struct {
    int roll_no;
    char name[50];
    char dept[10];
} StudentRecord;

/*
 * This struct is used to hold (key, rid) pairs for sorting
 * and bulk-loading.
 */
typedef struct {
    int roll_no;
    int packed_rid; /* (page_num << 16) | slot_num */
} StudentIndexEntry;


/*
 * Helper function to pack an SP_RID (page, slot) into a single integer
 * that the AM layer can use as its 'recId'.
 */
static int pack_rid(SP_RID rid) {
    /* Pack page number in high 16 bits, slot number in low 16 bits */
    return (rid.page_num << 16) | (rid.slot_num & 0xFFFF);
}

/*
 * Comparison function for qsort() to sort entries by roll_no.
 */
static int compare_student_entries(const void *a, const void *b) {
    StudentIndexEntry *entryA = (StudentIndexEntry*)a;
    StudentIndexEntry *entryB = (StudentIndexEntry*)b;
    return (entryA->roll_no - entryB->roll_no);
}

/*
 * Populates the student data file (using the SP layer).
 * If 'sorted' is true, it inserts records with sequential roll numbers.
 * If 'sorted' is false, it inserts records with random roll numbers.
 * Returns an array of StudentIndexEntry (which the caller must free)
 * containing the roll_no and packed_RID for each inserted record.
 */
static StudentIndexEntry* populate_student_file(int sp_fd, int num_records, int sorted) {
    StudentIndexEntry* entries = malloc(num_records * sizeof(StudentIndexEntry));
    if (entries == NULL) return NULL;
    
    StudentRecord rec;
    SP_RID rid;
    int i;

    printf("Populating %s with %d student records (%s)...\n", 
           STUDENT_FILE, num_records, sorted ? "SORTED" : "UNSORTED");
           
    for (i = 0; i < num_records; i++) {
        if (sorted) {
            rec.roll_no = 10000 + i;
        } else {
            /* Generate a random, unique-ish roll number */
            rec.roll_no = 10000 + (rand() % (num_records * 2));
        }
        sprintf(rec.name, "Student_%d", rec.roll_no);
        sprintf(rec.dept, "CS");
        
        if (SP_InsertRec(sp_fd, (char*)&rec, sizeof(StudentRecord), &rid) != SP_OK) {
            fprintf(stderr, "Failed to insert student record %d\n", i);
            free(entries);
            return NULL;
        }
        entries[i].roll_no = rec.roll_no;
        entries[i].packed_rid = pack_rid(rid);
    }
    printf("Population complete.\n");
    return entries;
}

#endif /* STUDENT_DATA_H */