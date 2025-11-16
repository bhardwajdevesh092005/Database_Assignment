#include "bl.h"
#include "blinternal.h"
static int sc(int k,int o,int s){
switch(o){case BL_EQ:return k==s;case BL_LT:return k<s;case BL_GT:return k>s;
case BL_LE:return k<=s;case BL_GE:return k>=s;case BL_NE:return k!=s;default:return 0;}}
int BL_OpenScan(int fd,int o,int sk,BL_ScanDesc*sd){
char*pb;BL_HeaderPage*h;if(PF_GetThisPage(fd,0,&pb,FALSE)!=PFE_OK)return BL_ERROR;
h=(BL_HeaderPage*)pb;sd->indexFd=fd;sd->op=o;sd->scanKey=sk;
sd->currentPage=h->firstLeaf;sd->currentSlot=0;sd->isOpen=1;
PF_UnfixPage(fd,0,FALSE);return BL_OK;}
int BL_GetNext(BL_ScanDesc*sd,int*k,int*v){
char*pb;BL_LeafHeader*lh;int*ks,*vs,e,ck,cv,np;
if(!sd->isOpen)return BL_ERROR;while(sd->currentPage!=-1){
if(PF_GetThisPage(sd->indexFd,sd->currentPage,&pb,FALSE)!=PFE_OK)return BL_ERROR;
lh=(BL_LeafHeader*)pb;ks=BL_LEAF_KEYS(pb);vs=BL_LEAF_VALUES(pb);
while(sd->currentSlot<lh->numKeys){ck=ks[sd->currentSlot];cv=vs[sd->currentSlot];
sd->currentSlot++;if(sc(ck,sd->op,sd->scanKey)){*k=ck;*v=cv;
PF_UnfixPage(sd->indexFd,sd->currentPage,FALSE);return BL_OK;}}
np=lh->nextPage;PF_UnfixPage(sd->indexFd,sd->currentPage,FALSE);
sd->currentPage=np;sd->currentSlot=0;}return BL_EOF;}
int BL_CloseScan(BL_ScanDesc*sd){sd->isOpen=0;return BL_OK;}
