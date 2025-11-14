/* splayer/ps.h
 *
 * Public API for the Slotted Page (SP) layer. This layer manages variable-length
 * records inside PF-managed pages using the slotted-page organization.
 *
 * The SP layer is a content-aware client of the PF layer. It uses PF_GetThisPage/
 * PF_AllocPage/PF_UnfixPage to access and modify pages. The file (on disk) is
 * a normal paged file created by the PF layer; the SP layer imposes the
 * slotted-page structure on individual pages.
 */
#ifndef SP_H
#define SP_H

#include "sptype.h"

/* Return codes: adopt PF layer error codes where applicable; we define a few
 * SP-specific return codes here as small positive/negative integers. Adjust
 * as needed to integrate with project conventions.
 */
#define SP_OK 0
#define SP_EOF 1
#define SP_ERROR (-1)
#define SP_NO_SLOT (-2)
#define SP_END_OF_LIST (-3)
#define SP_DELETED_LEN (-1)

void SP_Init(void);
/* Create/open/close wrappers (thin wrappers around PF functions) */
int SP_CreateFile(const char *fileName);
int SP_OpenFile(const char *fileName);
int SP_CloseFile(int fileDesc);

/* Record operations */
int SP_InsertRec(int fileDesc, const char *record, int length, SP_RID *rid);
int SP_DeleteRec(int fileDesc, SP_RID rid);
int SP_GetThisRec(int fileDesc, SP_RID rid, char *buffer, int bufSize);

/* Sequential scan helpers */
int SP_GetFirstRec(int fileDesc, char *buffer, int bufSize, SP_RID *rid);
int SP_GetNextRec(int fileDesc, SP_RID *currentRid, char *buffer, int bufSize, SP_RID *nextRid);

/* Utility: initialize a freshly allocated page as a slotted page
 * pageBuf: pointer returned by PF_AllocPage
 */
int SP_InitNewPage(char *pageBuf);

/* Performance and diagnostic functions */
int SP_GetFileStats(int fileDesc, SP_FileStats *stats);

#endif /* SP_H */
