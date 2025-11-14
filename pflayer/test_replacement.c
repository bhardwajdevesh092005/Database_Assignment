#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "pf.h"
#include "pftypes.h"

#define TEST_FILE "random_test.tmp"
#define ALLOC_SCALE_FACTOR 2.5 
#define ACCESS_COUNT (PF_MAX_BUFS * 5) 

/* These are assumed to be implemented in pf.c for stats */
extern void PF_InitStats();
extern void PF_PrintStats();
extern int physical_reads;
extern int physical_writes;

void run_randomized_workload(int policy) {
    int fd, i, pagenum;
    char *buf;
    const char* policy_str = (policy == 0) ? "LRU" : "MRU";
    int total_pages_to_alloc = (int)(PF_MAX_BUFS * ALLOC_SCALE_FACTOR);

    printf("\n====================================================\n");
    printf("  Running Randomized Workload Test for: %s\n", policy_str);
    printf("====================================================\n");

    /* 1. Setup */
    PF_CreateFile(TEST_FILE);
    if ((fd = PF_OpenFile(TEST_FILE, policy)) < 0) {
        PF_PrintError("open file");
        return;
    }
    PF_InitStats();

    /* 2. Allocation Phase: Allocate more pages than buffer size, forcing evictions. */
    printf("\n--- Allocation Phase ---\n");
    printf("Buffer size: %d. Allocating %d pages...\n", PF_MAX_BUFS, total_pages_to_alloc);
    for (i = 0; i < total_pages_to_alloc; i++) {
        if (PF_AllocPage(fd, &pagenum, &buf) != PFE_OK) {
            PF_PrintError("alloc page");
            goto cleanup;
        }
        sprintf(buf, "This is page %d", pagenum);
        /* Mark every page as dirty to see how many writes occur*/
        if (PF_UnfixPage(fd, pagenum, TRUE) != PFE_OK) {
            PF_PrintError("unfix page");
            goto cleanup;
        }
    }
    printf("Allocation complete. Stats for this phase:\n");
    PF_PrintStats();
    printf("Note: We expect %d physical writes because the first %d dirty pages were evicted.\n",
           total_pages_to_alloc - PF_MAX_BUFS, total_pages_to_alloc - PF_MAX_BUFS);


    /* 3. Random Access Phase: Access pages randomly, including ones on disk. */
    printf("\n--- Random Access Phase ---\n");
    printf("Performing %d random page accesses...\n", ACCESS_COUNT);
    PF_InitStats(); /* Reset stats for this phase */

    for (i = 0; i < ACCESS_COUNT; i++) {
        int page_to_access = rand() % total_pages_to_alloc;
        if (PF_GetThisPage(fd, page_to_access, &buf, FALSE) != PFE_OK) {
            PF_PrintError("get page");
            goto cleanup;
        }
        if (PF_UnfixPage(fd, page_to_access, FALSE) != PFE_OK) {
            PF_PrintError("unfix page");
            goto cleanup;
        }
    }
    printf("Random access complete. Stats for this phase:\n");
    PF_PrintStats();
    printf("Note: A higher hit rate indicates better performance for this random workload.\n");


cleanup:
    /* 4. Cleanup */
    printf("\n--- Cleanup ---\n");
    if (PF_CloseFile(fd) != PFE_OK) {
        PF_PrintError("close file");
    }
    PF_DestroyFile(TEST_FILE);
    printf("Test for %s complete.\n", policy_str);
}

int main() {
    srand(time(NULL));

    PF_Init();

    run_randomized_workload(0);
    run_randomized_workload(1);

    return 0;
}