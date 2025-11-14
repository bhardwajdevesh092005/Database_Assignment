#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "pf.h"
#include "pftypes.h"

#define TEST_FILE "mixed_workload.tmp"
#define NUM_PAGES_IN_FILE (PF_MAX_BUFS * 3) 
#define NUM_ACCESSES 1000 
#define NUM_RUNS_PER_TEST 5 

/* These are assumed to be implemented in pf.c for stats */
extern void PF_InitStats();
extern void PF_PrintStats();
extern int logical_reads;
extern int physical_reads;
extern int physical_writes;

/*
 * Represents the statistics for a single test run.
 */
typedef struct {
    int p_reads;
    int p_writes;
    float hit_rate;
} TestStats;

/*
 * Runs a workload with a specific mixture of reads and writes.
 */
TestStats run_mixed_workload(int policy, int read_percentage) {
    int fd, i, pagenum;
    char *buf;
    TestStats stats = {0, 0, 0.0};

    PF_CreateFile(TEST_FILE);
    if ((fd = PF_OpenFile(TEST_FILE, policy)) < 0) {
        PF_PrintError("open file");
        return stats;
    }

    for (i = 0; i < NUM_PAGES_IN_FILE; i++) {
        PF_AllocPage(fd, &pagenum, &buf);
        PF_UnfixPage(fd, pagenum, FALSE);
    }

    PF_InitStats();

    for (i = 0; i < NUM_ACCESSES; i++) {
        int page_to_access = rand() % NUM_PAGES_IN_FILE;
        int is_read = (rand() % 100) < read_percentage;

        if (PF_GetThisPage(fd, page_to_access, &buf, FALSE) != PFE_OK) {
            PF_PrintError("get page");
            goto cleanup;
        }

        if (is_read) {
            PF_UnfixPage(fd, page_to_access, FALSE);
        } else {
            sprintf(buf, "dirty data %d", i);
            PF_UnfixPage(fd, page_to_access, TRUE);
        }
    }

    stats.p_reads = physical_reads;
    stats.p_writes = physical_writes;
    if (logical_reads > 0) {
        stats.hit_rate = ((float)(logical_reads - physical_reads) / logical_reads) * 100.0;
    }

cleanup:
    PF_CloseFile(fd);
    PF_DestroyFile(TEST_FILE);
    return stats;
}

/*
 * Runs tests for various read/write mixtures and prints the averaged results.
 */
void run_and_average_workloads(int policy) {
    const char* policy_str = (policy == 0) ? "LRU" : "MRU";
    int percent;

    printf("\n===========================================================\n");
    printf("     Averaged Performance Results for: %s\n", policy_str);
    printf("===========================================================\n");
    printf("Read %% | Avg. Phys. Reads | Avg. Phys. Writes | Avg. Hit Rate %%\n");
    printf("-------|------------------|-------------------|----------------\n");

    for (percent = 10; percent <= 90; percent += 10) {
        long total_p_reads = 0;
        long total_p_writes = 0;
        float total_hit_rate = 0.0;
        int i;

        for (i = 0; i < NUM_RUNS_PER_TEST; i++) {
            TestStats current_stats = run_mixed_workload(policy, percent);
            total_p_reads += current_stats.p_reads;
            total_p_writes += current_stats.p_writes;
            total_hit_rate += current_stats.hit_rate;
        }

        printf("  %2d%%  | %16.2f | %17.2f | %13.2f%%\n",
               percent,
               (float)total_p_reads / NUM_RUNS_PER_TEST,
               (float)total_p_writes / NUM_RUNS_PER_TEST,
               total_hit_rate / NUM_RUNS_PER_TEST);
    }
}

int main() {
    srand(time(NULL));
    PF_Init();

    run_and_average_workloads(0);
    run_and_average_workloads(1);

    return 0;
}