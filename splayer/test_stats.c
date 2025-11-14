#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sp.h"
#include "sptype.h"
#include "pf.h"

#define STATS_TEST_FILE "stats_file.db"
#define NUM_RECORDS 1000

/* Helper function to check return codes */
void check_stats(int err, const char *message) {
    if (err != SP_OK) {
        fprintf(stderr, "ERROR: %s (code: %d)\n", message, err);
        exit(1);
    }
}

int main() {
    int fd;
    SP_RID rid;
    SP_FileStats sp_stats;
    long total_actual_bytes = 0;
    int i; /* C89 requires declarations at the top of the block */

    /* A sample of variable-length student data */
    const char *student_data[] = {
        "Babbage,Charles,CS,Senior",
        "Lovelace,Ada,MATH,Junior,Scholarship",
        "Turing,Alan,CS,PhD",
        "Hopper,Grace,PHYS,Senior",
        "Knuth,Donald,CS,Professor,Emeritus",
        "Von Neumann,John,MATH,Professor",
        "Ritchie,Dennis,CS,Alumnus"
    };
    int num_samples = sizeof(student_data) / sizeof(char*);

    /* --- Static Management Calculation Variables --- */
    int static_max_lengths[] = {50, 100, 200, 500, 1000, 2000};
    int num_static_cases = sizeof(static_max_lengths) / sizeof(int);
    float static_utilizations[num_static_cases];
    int static_total_pages[num_static_cases];

    printf("--- Slotted Page Performance Test ---\n");

    /* Initialize Layers */
    SP_Init();
    PF_InitStats();

    /* Create and open the file */
    check_stats(SP_CreateFile(STATS_TEST_FILE), "Create file failed");
    fd = SP_OpenFile(STATS_TEST_FILE);
    if (fd < 0) check_stats(fd, "Open file failed");

    printf("Inserting %d variable-length records...\n", NUM_RECORDS);
    for (i = 0; i < NUM_RECORDS; i++) {
        const char *record = student_data[rand() % num_samples];
        int len = strlen(record) + 1;
        total_actual_bytes += len;
        check_stats(SP_InsertRec(fd, record, len, &rid), "Insert record failed");
    }
    printf("Insertion complete.\n\n");

    /* --- Gather Slotted Page Statistics --- */
    printf("Gathering slotted page file statistics...\n");
    check_stats(SP_GetFileStats(fd, &sp_stats), "GetFileStats failed");

    /* --- Calculate Static Management Statistics --- */
    for (i = 0; i < num_static_cases; i++) {
        int max_len = static_max_lengths[i];
        int recs_per_page = floor((float)PF_PAGE_SIZE / max_len);
        int pages_needed;
        long total_static_bytes;

        if (recs_per_page == 0) recs_per_page = 1;
        pages_needed = ceil((float)NUM_RECORDS / recs_per_page);
        total_static_bytes = pages_needed * PF_PAGE_SIZE;
        static_utilizations[i] = ((float)total_actual_bytes / total_static_bytes) * 100.0f;
        static_total_pages[i] = pages_needed;
    }

    /* --- Print Results Table --- */
    printf("\n--- Performance Comparison Report ---\n\n");
    printf("Total Records Inserted: %d\n", NUM_RECORDS);
    printf("Total Bytes of Actual Data: %ld\n\n", total_actual_bytes);

    printf("| Management Strategy         | Total Pages | Space Utilization |\n");
    printf("|-----------------------------|-------------|-------------------|\n");
    printf("| Slotted Page (Our Impl.)    | %-11d | %.2f%%            |\n", sp_stats.total_pages, sp_stats.utilization);
    for (i = 0; i < num_static_cases; i++) {
        char strategy_name[50];
        sprintf(strategy_name, "Static (Max Rec = %d)", static_max_lengths[i]);
        printf("| %-27s | %-11d | %.2f%%            |\n", strategy_name, static_total_pages[i], static_utilizations[i]);
    }

    printf("\n--- PF Layer I/O Statistics ---\n");
    PF_PrintStats();

    /* Cleanup */
    check_stats(SP_CloseFile(fd), "Close file failed");
    check_stats(SP_DestroyFile(STATS_TEST_FILE), "Destroy file failed");

    return 0;
}