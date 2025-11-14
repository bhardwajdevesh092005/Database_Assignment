# Project Progress Report: PF Layer Buffering and Replacement Strategies

This report details the implementation progress toward the project objective of creating a buffered paged file (PF) layer with selectable replacement strategies and performance monitoring.

## Objective Summary

The goal is to implement a robust page buffering system for the PF layer with the following key features:
-   **Dual Replacement Strategies:** Support for both LRU (Least Recently Used) and MRU (Most Recently Used) page replacement policies.
-   **Selectable Strategy:** The desired strategy (LRU/MRU) must be selectable when a file is opened.
-   **Dirty Flag Management:** Pages modified in the buffer must be tracked with a "dirty" flag to ensure they are written back to disk.
-   **Statistics Collection:** The system must track and report performance metrics, including logical I/O, physical I/O (reads and writes), and buffer hit rate.

## Implementation Analysis and Current Progress

Based on the recent changes, significant progress has been made towards fulfilling these objectives.

### 1. Selectable Replacement Strategies (LRU/MRU)

The core mechanism for switching between LRU and MRU has been implemented.

-   **Strategy Selection:** The `PF_OpenFile` function in `pf.c` has been modified to accept an integer `policy` argument. This value is stored in a global `bufferPolicy` variable, making the chosen strategy available to the buffer management logic.

-   **Victim Selection Logic:** The buffer allocation function (`PFbufInternalAlloc` in `buf.c`) now contains the critical logic for victim selection. A `for` loop intelligently traverses the buffer's linked list based on the `bufferPolicy`:
    -   If the policy is **LRU**, the loop starts from the tail (`PFlastbpage`) and moves backward, looking for the first unpinned page to evict.
    -   If the policy is **MRU**, the loop starts from the head (`PFfirstbpage`) and moves forward.

    This implementation correctly identifies the appropriate victim page according to the selected strategy.

### 2. Dirty Flag and Write-Back Logic

The system correctly handles dirty pages to prevent data loss.

-   **Marking Pages Dirty:** The `PF_UnfixPage` function allows a page to be marked as dirty, which is essential for tracking modifications.
-   **Write-on-Eviction:** Inside `PFbufInternalAlloc`, before a victim page is evicted, its `dirty` flag is checked. If the flag is true, the page's contents are written back to disk. This ensures that all changes are persisted.

### 3. Statistics Collection Framework

A framework for collecting performance statistics has been successfully integrated.

-   **Counters:** Global static counters for `logical_reads`, `physical_reads`, and `physical_writes` have been added to `pf.c`.
-   **Instrumentation:**
    -   A **logical read** is correctly counted every time `PF_GetPage` is called, representing a request for a page.
    -   A **physical read** is counted within `PF_GetPage` only when the requested page is not found in the buffer, forcing a read from disk.
    -   A **physical write** is counted in `PFbufInternalAlloc` when a dirty page is written to disk during eviction.
-   **Reporting:** `PF_InitStats()` and `PF_PrintStats()` functions have been created to manage and display the collected statistics, including a calculated buffer hit rate.

## Performance Test Results

The `test_lru_mru` executable was run to generate performance data for both the LRU and MRU replacement strategies under various read/write mixtures. The following tables summarize the averaged results over 5 runs for each workload.

### LRU Strategy Performance

| Read % | Avg. Phys. Reads | Avg. Phys. Writes | Avg. Hit Rate % |
| :----: | :--------------: | :---------------: | :-------------: |
|  10%   |      672.00      |      621.80       |     32.80%      |
|  20%   |      674.00      |      576.60       |     32.60%      |
|  30%   |      675.40      |      531.60       |     32.46%      |
|  40%   |      665.40      |      470.20       |     33.46%      |
|  50%   |      664.80      |      401.80       |     33.52%      |
|  60%   |      663.40      |      344.40       |     33.66%      |
|  70%   |      668.00      |      271.00       |     33.20%      |
|  80%   |      663.00      |      199.00       |     33.70%      |
|  90%   |      678.80      |      109.00       |     32.12%      |

### MRU Strategy Performance

| Read % | Avg. Phys. Reads | Avg. Phys. Writes | Avg. Hit Rate % |
| :----: | :--------------: | :---------------: | :-------------: |
|  10%   |      675.00      |      628.40       |     32.50%      |
|  20%   |      677.60      |      572.20       |     32.24%      |
|  30%   |      666.20      |      520.60       |     33.38%      |
|  40%   |      670.00      |      465.00       |     33.00%      |
|  50%   |      674.00      |      403.20       |     32.60%      |
|  60%   |      670.40      |      335.60       |     32.96%      |
|  70%   |      666.20      |      263.80       |     33.38%      |
|  80%   |      665.00      |      189.40       |     33.50%      |
|  90%   |      669.40      |      111.80       |     33.06%      |

### Analysis of Results

Under a purely random access pattern, both LRU and MRU exhibit very similar performance. The buffer hit rate for both strategies hovers around 33%, which is what one would expect when the pages being accessed have poor temporal locality (i.e., accessing one page gives no information about what page will be accessed next).

-   **Physical Reads:** The number of physical reads is consistently high for both policies, as the random access pattern frequently requests pages that have been evicted from the buffer.
-   **Physical Writes:** As the percentage of read queries increases, the number of write operations naturally decreases, leading to a steady decline in physical writes for both strategies.

These results establish a baseline for random workloads. The key difference between LRU and MRU is expected to appear in workloads with more predictable access patterns, such as sequential scans, which will be the focus of future tests.

## Performance Visualization

The following graph visually represents the data from the tables above, comparing the physical I/O counts and buffer hit rates for both LRU and MRU strategies across different read/write workload mixtures.

![Performance of LRU vs. MRU under Randomized Workloads](performance_graph.png)

*Comparison of LRU and MRU performance metrics. The bars represent the average number of physical reads and writes, while the line graph shows the buffer hit rate. As shown, performance is nearly identical for random workloads.*
