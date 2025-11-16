#!/bin/bash
# Script to create all bulklayer source files

echo "Creating bl.c..."
cat > bl.c << 'EOFC1'
#include "bl.h"
#include "blinternal.h"
#include <stdio.h>
#include <string.h>

void BL_Init(void) {
    PF_Init();
}

int BL_CreateIndex(char *fileName) {
    int fd, err, pageNum;
    char *pageBuf;
    BL_HeaderPage *header;
    
    err = PF_CreateFile(fileName);
    if (err != PFE_OK) {
        PF_PrintError("BL_CreateIndex failed");
        return BL_ERROR;
    }
    
    fd = PF_OpenFile(fileName, 0);
    if (fd < 0) return BL_ERROR;
    
    err = PF_AllocPage(fd, &pageNum, &pageBuf);
    if (err != PFE_OK) {
        PF_CloseFile(fd);
        return BL_ERROR;
    }
    
    header = (BL_HeaderPage*)pageBuf;
    header->rootPage = -1;
    header->firstLeaf = -1;
    header->numLevels = 0;
    header->numRecords = 0;
    header->keyType = 'i';
    header->keyLength = sizeof(int);
    
    PF_UnfixPage(fd, 0, TRUE);
    PF_CloseFile(fd);
    return BL_OK;
}

int BL_DestroyIndex(char *fileName) {
    return (PF_DestroyFile(fileName) == PFE_OK) ? BL_OK : BL_ERROR;
}

int BL_OpenIndex(char *fileName) {
    int fd = PF_OpenFile(fileName, 0);
    return (fd < 0) ? BL_ERROR : fd;
}

int BL_CloseIndex(int indexFd) {
    return (PF_CloseFile(indexFd) == PFE_OK) ? BL_OK : BL_ERROR;
}
EOFC1

echo "bl.c created"
echo "Now compile manually with: make"
