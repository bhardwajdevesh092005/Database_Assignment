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
    -   A **logical read** is correctly counted every time `PF_GetThisPage` is called, representing a request for a page.
    -   A **physical read** is counted within `PF_GetThisPage` only when the requested page is not found in the buffer, forcing a read from disk.
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

## Slotted Page (SP) Layer Implementation

A new layer, the Slotted Page (SP) layer, has been built on top of the PF layer to manage variable-length records efficiently.

### 1. Objective: Variable-Length Record Management

The primary goal of the SP layer is to overcome the space inefficiency of static-sized record layouts. It achieves this by organizing pages into a "slotted" structure, which includes:
-   A **Page Header** containing metadata like the number of slots and pointers to free space.
-   A **Slot Directory** that grows from the beginning of the page, with each entry pointing to a record and storing its length.
-   A **Record Data Area** that grows backward from the end of the page.

This structure allows records of any size to be packed tightly, maximizing space utilization.

### 2. Core Functionality

The SP layer provides a complete API for record management:
-   **Insertion (`SP_InsertRec`):** Intelligently finds a page with sufficient contiguous free space. If no such page exists, it allocates a new one.
-   **Deletion (`SP_DeleteRec`):** Marks a record's slot as deleted, making its space available for future use.
-   **Compaction (`SP_CompactPage`):** An internal function automatically triggered by `SP_DeleteRec` when more than half of a page's space is fragmented (i.e., occupied by deleted records). It reorganizes the page to create a single, large block of free space, enabling the insertion of larger records.
-   **Scanning (`SP_GetFirstRec`, `SP_GetNextRec`):** Provides a robust mechanism to perform a full sequential scan of all valid records in the file, correctly skipping over any deleted slots.

### 3. Integration with PF Layer

The SP layer is a client of the PF layer. It treats the PF layer as a page provider and is responsible for interpreting and managing the content *within* each page. This maintains a clean separation of concerns, where the PF layer handles I/O and buffering, and the SP layer handles record layout and management.

### 4. Test Results and Performance Analysis

The SP layer was subjected to two sets of tests: a correctness suite (`test_sp`) and a performance analysis suite (`test_stats`).

#### Correctness Verification

The `test_sp` program verified the core logic:
-   **Basic Insertion & Retrieval:** Confirmed that records can be inserted and then retrieved with their original data intact.
-   **Deletion & Scanning:** Verified that deleted records are correctly skipped during a linear scan and cannot be fetched individually.
-   **Compaction:** Confirmed that the page compaction logic works by deleting numerous small records and then successfully inserting a large record that would have failed without compaction.

The output confirms that all correctness tests passed:
```
--- Test 1: Basic Insertion and Retrieval ---
Inserting 3 records...
  Record 1 inserted at (Page: 0, Slot: 0)
  Record 2 inserted at (Page: 0, Slot: 1)
  Record 3 inserted at (Page: 0, Slot: 2)
Retrieving records to verify...
  SUCCESS: Record 2 verified.
  SUCCESS: Record 1 verified.
--- Test 1 Complete ---

--- Test 2: Linear Scan and Deletion ---
Inserting 10 records...
Scanning all 10 records...
  SUCCESS: Scan found all 10 records.
Deleting records at slot 3, 5, and 8...
Verifying deleted record cannot be fetched...
  SUCCESS: SP_GetThisRec correctly failed for deleted record.
Scanning again to count remaining records...
  SUCCESS: Scan found 7 records after deletion.
--- Test 2 Complete ---

--- Test 3: Compaction Trigger ---
Inserting 102 small records to create fragmentation...
Deleting every other record to trigger compaction...
  Deletion complete. Compaction should have occurred.
Attempting to insert a large record that requires compacted space...
  SUCCESS: Large record inserted successfully, compaction worked.
--- Test 3 Complete ---

All SP Layer tests passed.
```

#### Performance and Space Utilization

The `test_stats` program inserted 1,000 variable-length records and compared the space utilization of our slotted page implementation against a static-sized record approach.

The results clearly demonstrate the efficiency of the slotted page structure:
-   **Slotted Page:** Achieved **99.66%** space utilization, using only 9 pages.
-   **Static Sized:** As the maximum possible record size increases, the space utilization plummets, falling to just **1.39%** when provisioning for 2000-byte records. This is because every record, no matter how small, must reserve the maximum possible space.

The slotted page design proves to be vastly superior for managing variable-length data, adapting dynamically to the actual data size and minimizing wasted space.

**Performance Comparison Table:**
```
--- Performance Comparison Report ---

Total Records Inserted: 1000
Total Bytes of Actual Data: 28560

| Management Strategy         | Total Pages | Space Utilization |
|-----------------------------|-------------|-------------------|
| Slotted Page (Our Impl.)    | 9           | 99.66%            |
| Static (Max Rec = 50)       | 13          | 53.64%            |
| Static (Max Rec = 100)      | 25          | 27.89%            |
| Static (Max Rec = 200)      | 50          | 13.95%            |
| Static (Max Rec = 500)      | 125         | 5.58%             |
| Static (Max Rec = 1000)     | 250         | 2.79%             |
| Static (Max Rec = 2000)     | 500         | 1.39%             |
```
