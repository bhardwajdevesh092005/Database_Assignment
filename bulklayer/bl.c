#include "bl.h"
#include "blinternal.h"
#include <stdio.h>

void BL_Init(void) { PF_Init(); }
int BL_CreateIndex(char *f) {
    int fd, err, p; char *b; BL_HeaderPage *h;
    if(PF_CreateFile(f)!=PFE_OK) return BL_ERROR;
    if((fd=PF_OpenFile(f,0))<0) return BL_ERROR;
    if(PF_AllocPage(fd,&p,&b)!=PFE_OK){PF_CloseFile(fd);return BL_ERROR;}
    h=(BL_HeaderPage*)b;h->rootPage=-1;h->firstLeaf=-1;h->numLevels=0;
    h->numRecords=0;h->keyType='i';h->keyLength=sizeof(int);
    PF_UnfixPage(fd,0,TRUE);PF_CloseFile(fd);return BL_OK;
}
int BL_DestroyIndex(char *f){return PF_DestroyFile(f)==PFE_OK?BL_OK:BL_ERROR;}
int BL_OpenIndex(char *f){int fd=PF_OpenFile(f,0);return fd<0?BL_ERROR:fd;}
int BL_CloseIndex(int fd){return PF_CloseFile(fd)==PFE_OK?BL_OK:BL_ERROR;}
