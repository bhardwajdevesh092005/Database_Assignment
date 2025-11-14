/* splayer/sptype.h
 *
 * Definitions for the Slotted Page (SP) layer types.
 * A slotted page is the in-page structure used to store variable-length
 * records. This header defines the page header, slot entry and RID types
 * used by the SP layer.
 */
#ifndef SP_TYPE_H
#define SP_TYPE_H

#include "pf.h" /* PF_PAGE_SIZE and PF layer types */

/* Record Identifier (RID): identifies a record by page number and slot number */
typedef struct SP_RID {
    int page_num;   /* page number within the PF file */
    int slot_num;   /* slot index within the page */
} SP_RID;

/* Slotted page header stored at the beginning of each data page.
 * Layout notes:
 * - header is at offset 0 of the page
 * - slot directory immediately follows the header and grows forward
 * - record data grows backward from the end of the page
 */
typedef struct SP_PageHeader {
    int num_slots;      /* total number of slots in the directory */
    int slot_count;     /* number of currently occupied slots */
    int free_space;     /* offset from start of page to start of contiguous free space */
    int total_free_space; /* contiguous free space + space from deleted records */
    int reserved;       /* reserved for alignment / future flags */
} SP_PageHeader;

/* A slot directory entry. Each slot points to a record's offset and stores
// ...existing code...
 */
typedef struct SP_Slot {
    int length; /* record length; -1 if slot is empty (SP_DELETED_LEN) */
    int offset; /* offset from start of page to the record's first byte */
} SP_Slot;

/* Structure to hold file-level statistics */
typedef struct SP_FileStats {
    int total_pages;
    int total_records;
    long record_bytes;  /* Total bytes used by actual record data */
    long overhead_bytes;/* Total bytes used by headers and slot directories */
    float utilization;  /* Overall space utilization percentage */
} SP_FileStats;

/* Convenience macros */
#define SP_DELETED_LEN (-1)
/* Compute the starting offset of the slot directory for a page buffer */
/* slot directory immediately follows the page header */
#define SP_SLOT_DIR_OFFSET() (sizeof(SP_PageHeader))

/* Maximum usable space inside a page for slot directory + record data */
#define SP_PAGE_USABLE_SPACE (PF_PAGE_SIZE - sizeof(SP_PageHeader))

#endif /* SP_TYPE_H */
