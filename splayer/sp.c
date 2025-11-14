#include "sp.h"
#include "sptype.h"
#include "pf.h"
#include "pftypes.h" /* For LRU/MRU constants */
#include <string.h>
#include <stdio.h> /* For TRUE/FALSE, NULL */

void SP_Init()
/****************************************************************************
SPECIFICATIONS:
    Initialize the Slotted Page layer. This function should be called
    before any other SP layer functions are used.
AUTHOR: Devesh Bhardwaj
RETURN VALUE:
    None.
*****************************************************************************/
{
    PF_Init();
}

int SP_CreateFile(const char *fileName)
/****************************************************************************
SPECIFICATIONS:
    Create a new slotted page file with the name "fileName". This function
    creates a paged file using the PF layer, and initializes the file header.
AUTHOR: Devesh Bhardwaj
RETURN VALUE:
    SP_OK if successful.
    SP_ERROR if unsuccessful.
*****************************************************************************/
{
    return PF_CreateFile(fileName);
}

int SP_OpenFile(const char *fileName)
/****************************************************************************
SPECIFICATIONS:
    Open an existing slotted page file with the name "fileName". This function
    opens the paged file using the PF layer.
AUTHOR: Devesh Bhardwaj
RETURN VALUE:
    File descriptor if successful.
    SP_ERROR if unsuccessful.
*****************************************************************************/
{
    /* Default to LRU for the SP layer, as record access patterns can be random */
    return PF_OpenFile(fileName, 0);
}

int SP_CloseFile(int fileDesc)
/****************************************************************************
SPECIFICATIONS:
    Close the slotted page file identified by "fileDesc". This function
    closes the paged file using the PF layer.
AUTHOR: Devesh Bhardwaj
RETURN VALUE:
    SP_OK if successful.
    SP_ERROR if unsuccessful.
*****************************************************************************/
{
    return PF_CloseFile(fileDesc);
}

int SP_DestroyFile(const char *fileName)
/****************************************************************************
SPECIFICATIONS:
    Destroy the slotted page file with the name "fileName". This function
    deletes the paged file using the PF layer.
AUTHOR: Devesh Bhardwaj
RETURN VALUE:
    SP_OK if successful.
    SP_ERROR if unsuccessful.
*****************************************************************************/
{
    return PF_DestroyFile(fileName);
}

/*
 * ================================================================================
 *                        Page-Level Initialization
 * ================================================================================
 */

int SP_InitNewPage(char *pageBuf) {
    if (pageBuf == NULL) return SP_ERROR;

    SP_PageHeader *header = (SP_PageHeader*)pageBuf;
    header->num_slots = 0;
    header->slot_count = 0;
    header->free_space = PF_PAGE_SIZE;
    header->total_free_space = PF_PAGE_SIZE - sizeof(SP_PageHeader);
    header->reserved = 0;

    return SP_OK;
}

/*
 * ================================================================================
 *                        Internal Compaction Function
 * ================================================================================
 */
static int SP_CompactPage(char *pageBuf) {
    char temp_page[PF_PAGE_SIZE];
    SP_PageHeader *header = (SP_PageHeader*)pageBuf;
    SP_Slot *slots = (SP_Slot*)(pageBuf + sizeof(SP_PageHeader));
    int new_free_ptr = PF_PAGE_SIZE;
    int i;

    /* Iterate through slots and copy live records to a temporary page */
    for (i = 0; i < header->num_slots; i++) {
        if (slots[i].length != SP_DELETED_LEN) {
            int len = slots[i].length;
            int old_offset = slots[i].offset;
            
            /* Calculate new position and copy data */
            new_free_ptr -= len;
            memcpy(temp_page + new_free_ptr, pageBuf + old_offset, len);
            
            /* Update the slot's offset in the original buffer */
            slots[i].offset = new_free_ptr;
        }
    }

    /* Copy the header and updated slot directory to the temporary page */
    memcpy(temp_page, pageBuf, sizeof(SP_PageHeader) + header->num_slots * sizeof(SP_Slot));

    /* Copy the entire compacted temporary page back to the original buffer */
    memcpy(pageBuf, temp_page, PF_PAGE_SIZE);

    /* Update the header with new free space pointers */
    header->free_space = new_free_ptr;
    header->total_free_space = header->free_space - (sizeof(SP_PageHeader) + (header->num_slots * sizeof(SP_Slot)));

    return SP_OK;
}


/*
 * ================================================================================
 *                        Record Insertion
 * ================================================================================
 */
int SP_InsertRec(int fileDesc, const char *record, int length, SP_RID *rid) {
    int pageNum = -1;
    char *pageBuf = NULL;
    int err;
    int required_space = length + sizeof(SP_Slot);

    
    /*
     * ------------------------------------------------------------------------
     *   Phase 1: Find a page with enough space for the new record and its slot.
     * ------------------------------------------------------------------------
     */
    while (TRUE) {
        pageNum++;
        err = PF_GetThisPage(fileDesc, pageNum, &pageBuf, FALSE);

        if (err == PFE_OK) {
            /* A page was found, check if it has enough space */
            SP_PageHeader *header = (SP_PageHeader*)pageBuf;
            int available_space = header->free_space - (sizeof(SP_PageHeader) + (header->num_slots * sizeof(SP_Slot)));

            if (available_space >= required_space) {
                /* Found a suitable page, break the loop to proceed with insertion */
                break;
            } else {
                /* Not enough space, unfix this page and continue to the next one */
                PF_UnfixPage(fileDesc, pageNum, FALSE);
                pageBuf = NULL; /* Reset pageBuf to indicate we are done with this page */
            }
        } else if (err == PFE_EOF || err == PFE_INVALIDPAGE) {
            /* Reached the end of the file, no existing page has enough space. */
            /* Break the loop to allocate a new page. */
            break;
        } else {
            /* An unexpected error occurred */
            PF_PrintError("SP_InsertRec: PF_GetThisPage failed");
            return SP_ERROR;
        }
    }

    /*
     * ------------------------------------------------------------------------
     *   Phase 2: Allocate a new page if no suitable page was found.
     * ------------------------------------------------------------------------
     */
    if (pageBuf == NULL) {
        err = PF_AllocPage(fileDesc, &pageNum, &pageBuf);
        if (err != PFE_OK) {
            PF_PrintError("SP_InsertRec: PF_AllocPage failed");
            return SP_ERROR; /* Failed to allocate a new page */
        }
        /* Format the new page with a slotted page header */
        SP_InitNewPage(pageBuf);
    }
     /* ------------------------------------------------------------------------
     *   Phase 3: Perform the insertion on the pinned page (held in pageBuf).
     * ------------------------------------------------------------------------
     */
    SP_PageHeader *header = (SP_PageHeader*)pageBuf;
    SP_Slot *slots = (SP_Slot*)(pageBuf + sizeof(SP_PageHeader));
    
    /* With this simple strategy, we always add a new slot */
    int slot_idx = header->num_slots;

    /* Calculate new data offset and copy the record */
    int new_offset = header->free_space - length;
    memcpy(pageBuf + new_offset, record, length);

    /* Update the slot */
    slots[slot_idx].length = length;
    slots[slot_idx].offset = new_offset;

    /* Update the page header */
    header->free_space = new_offset;
    header->num_slots++;
    header->slot_count++;
    header->total_free_space -= (length + sizeof(SP_Slot));
    
    PF_UnfixPage(fileDesc, pageNum, TRUE);
    
    rid->page_num = pageNum;
    rid->slot_num = slot_idx;

    return SP_OK;
}


/*
 * ================================================================================
 *                        Record Deletion
 * ================================================================================
 */
int SP_DeleteRec(int fileDesc, SP_RID rid) {
    char *pageBuf;
    int err;

    err = PF_GetThisPage(fileDesc, rid.page_num, &pageBuf, FALSE);
    if (err != PFE_OK) {
        PF_PrintError("SP_DeleteRec: PF_GetThisPage failed");
        return SP_ERROR;
    }

    SP_PageHeader *header = (SP_PageHeader*)pageBuf;
    SP_Slot *slot = (SP_Slot*)(pageBuf + sizeof(SP_PageHeader)) + rid.slot_num;

    /* Get length before marking as deleted */
    int deleted_len = slot->length;

    if (deleted_len != SP_DELETED_LEN) {
        slot->length = SP_DELETED_LEN;
        header->slot_count--;
        header->total_free_space += deleted_len;

        /* Check if compaction is needed */
        if (header->total_free_space > (PF_PAGE_SIZE / 2)) {
            SP_CompactPage(pageBuf);
        }
    }

    PF_UnfixPage(fileDesc, rid.page_num, TRUE);

    return SP_OK;
}
int SP_GetThisRec(int fileDesc, SP_RID rid, char *buffer, int bufSize) {
    char *pageBuf;
    int err;

    /* --- 1. Input Validation --- */
    if (buffer == NULL) {
        fprintf(stderr, "SP_GetThisRec: Provided buffer is NULL.\n");
        return SP_ERROR;
    }
    if (rid.page_num < 0 || rid.slot_num < 0) {
        fprintf(stderr, "SP_GetThisRec: Invalid RID (negative page or slot number).\n");
        return SP_ERROR;
    }

    /* --- 2. Get the Page from the PF Layer --- */
    err = PF_GetThisPage(fileDesc, rid.page_num, &pageBuf, FALSE);
    if (err != PFE_OK) {
        /* Let the PF layer print the specific error */
        PF_PrintError("SP_GetThisRec: PF_GetThisPage failed");
        return SP_ERROR;
    }

    /* --- 3. Access Page Internals and Validate Slot --- */
    SP_PageHeader *header = (SP_PageHeader*)pageBuf;
    SP_Slot *slots = (SP_Slot*)(pageBuf + sizeof(SP_PageHeader));

    /* Edge Case: Is the slot number valid for this page? */
    if (rid.slot_num >= header->num_slots) {
        fprintf(stderr, "SP_GetThisRec: Slot number %d is out of bounds for page %d.\n", rid.slot_num, rid.page_num);
        PF_UnfixPage(fileDesc, rid.page_num, FALSE); /* Unfix before returning */
        return SP_ERROR;
    }

    SP_Slot *slot = &slots[rid.slot_num];

    /* Edge Case: Has the record in this slot been deleted? */
    if (slot->length == SP_DELETED_LEN) {
        fprintf(stderr, "SP_GetThisRec: Record at page %d, slot %d has been deleted.\n", rid.page_num, rid.slot_num);
        PF_UnfixPage(fileDesc, rid.page_num, FALSE);
        return SP_ERROR;
    }

    /* Edge Case: Is the user's buffer large enough? */
    if (bufSize < slot->length) {
        fprintf(stderr, "SP_GetThisRec: Provided buffer (size %d) is too small for record (size %d).\n", bufSize, slot->length);
        PF_UnfixPage(fileDesc, rid.page_num, FALSE);
        return SP_ERROR;
    }

    /* --- 4. Copy the Record Data --- */
    memcpy(buffer, pageBuf + slot->offset, slot->length);

    /* --- 5. Cleanup --- */
    /* Unfix the page. Since we only read from it, the dirty flag is FALSE. */
    err = PF_UnfixPage(fileDesc, rid.page_num, FALSE);
    if (err != PFE_OK) {
        PF_PrintError("SP_GetThisRec: PF_UnfixPage failed");
        return SP_ERROR;
    }

    return SP_OK;
}

/*
 * ================================================================================
 *                        Sequential Scan Functions
 * ================================================================================
 */

/**
 * Finds the first valid record in the file.
 * This is a convenience wrapper around SP_GetNextRec.
 */
int SP_GetFirstRec(int fileDesc, char *buffer, int bufSize, SP_RID *rid) {
    /* Create a "dummy" RID that points to the position just before the first record */
    SP_RID currentRid;
    currentRid.page_num = 0;
    currentRid.slot_num = -1;

    /* Start the scan from this initial position */
    return SP_GetNextRec(fileDesc, &currentRid, buffer, bufSize, rid);
}

/**
 * Finds the next valid record in the file after the one specified by currentRid.
 */
int SP_GetNextRec(int fileDesc, SP_RID *currentRid, char *buffer, int bufSize, SP_RID *nextRid) {
    char *pageBuf;
    int err;
    int pageNum;
    int slotNum;

    /* --- 1. Input Validation --- */
    if (currentRid == NULL || buffer == NULL || nextRid == NULL) {
        fprintf(stderr, "SP_GetNextRec: Received NULL pointer for arguments.\n");
        return SP_ERROR;
    }

    /* --- 2. The Main Scan Loop --- */
    /* Start scanning from the page of the current RID */
    for (pageNum = currentRid->page_num; ; pageNum++) {
        err = PF_GetThisPage(fileDesc, pageNum, &pageBuf, FALSE);

        if (err == PFE_EOF) {
            /* We've scanned past the last page in the file. */
            return SP_EOF;
        } else if (err != PFE_OK) {
            PF_PrintError("SP_GetNextRec: PF_GetThisPage failed");
            return SP_ERROR;
        }

        /* We have a valid page, now scan its slots */
        SP_PageHeader *header = (SP_PageHeader*)pageBuf;
        SP_Slot *slots = (SP_Slot*)(pageBuf + sizeof(SP_PageHeader));

        /* Determine the starting slot for this page */
        if (pageNum == currentRid->page_num) {
            /* On the first page, start one slot after the current one */
            slotNum = currentRid->slot_num + 1;
        } else {
            /* On subsequent pages, always start from the first slot */
            slotNum = 0;
        }

        for (; slotNum < header->num_slots; slotNum++) {
            SP_Slot *slot = &slots[slotNum];

            /* Check if this slot contains a valid, non-deleted record */
            if (slot->length != SP_DELETED_LEN) {
                /* Found a valid record! */

                /* Edge Case: Is the user's buffer large enough? */
                if (bufSize < slot->length) {
                    fprintf(stderr, "SP_GetNextRec: Provided buffer is too small.\n");
                    PF_UnfixPage(fileDesc, pageNum, FALSE);
                    return SP_ERROR;
                }

                /* Copy the record data and set the RID for the user */
                memcpy(buffer, pageBuf + slot->offset, slot->length);
                nextRid->page_num = pageNum;
                nextRid->slot_num = slotNum;

                /* Unfix the page and return success */
                PF_UnfixPage(fileDesc, pageNum, FALSE);
                return SP_OK;
            }
        }

        /* If we finish the slot loop, no record was found on this page. */
        /* Unfix the page and let the outer loop continue to the next page. */
        PF_UnfixPage(fileDesc, pageNum, FALSE);
    }

    /* This line should be unreachable */
    return SP_EOF;
}

/*
 * ================================================================================
 *                        Performance and Statistics
 * ================================================================================
 */
int SP_GetFileStats(int fileDesc, SP_FileStats *stats) {
    char *pageBuf;
    int pageNum = -1;
    int err;

    /* Initialize stats struct */
    memset(stats, 0, sizeof(SP_FileStats));

    while (TRUE) {
        pageNum++;
        err = PF_GetThisPage(fileDesc, pageNum, &pageBuf, FALSE);

        if (err == PFE_EOF || err == PFE_INVALIDPAGE) {
            break; /* End of file, scan is complete */
        } else if (err != PFE_OK) {
            PF_PrintError("SP_GetFileStats: PF_GetThisPage failed");
            return SP_ERROR;
        }

        stats->total_pages++;
        SP_PageHeader *header = (SP_PageHeader*)pageBuf;
        SP_Slot *slots = (SP_Slot*)(pageBuf + sizeof(SP_PageHeader));

        stats->overhead_bytes += sizeof(SP_PageHeader) + (header->num_slots * sizeof(SP_Slot));
        int i;
        for (i = 0; i < header->num_slots; i++) {
            if (slots[i].length != SP_DELETED_LEN) {
                stats->total_records++;
                stats->record_bytes += slots[i].length;
            }
        }

        PF_UnfixPage(fileDesc, pageNum, FALSE);
    }

    if (stats->total_pages > 0) {
        long total_used_bytes = stats->record_bytes + stats->overhead_bytes;
        long total_available_bytes = stats->total_pages * PF_PAGE_SIZE;
        stats->utilization = ((float)total_used_bytes / total_available_bytes) * 100.0f;
    }

    return SP_OK;
}