#include "bl.h"
#include "blinternal.h"
#include <stdlib.h>
typedef struct{int*p;int*k;int n;int c;}L;
L*cL(){L*l=malloc(sizeof(L));l->c=100;l->n=0;l->p=malloc(l->c*sizeof(int));l->k=malloc(l->c*sizeof(int));return l;}
void eL(L*l){if(l->n>=l->c){l->c*=2;l->p=realloc(l->p,l->c*sizeof(int));l->k=realloc(l->k,l->c*sizeof(int));}}
void fL(L*l){free(l->p);free(l->k);free(l);}
int BL_BulkLoad(int fd,BL_KeyVal*d,int nr){
char*pb;int pn,i,e,cp=-1,nk=0,fp=-1,nl=0;L*cl,*pl;BL_HeaderPage*h;
if(!nr)return BL_OK;cl=cL();
for(i=0;i<nr;i++){if(nk>=BL_MAX_LEAF_ENTRIES||cp==-1){
if(cp!=-1){BL_LeafHeader*lh=(BL_LeafHeader*)pb;lh->numKeys=nk;lh->nextPage=-1;
eL(cl);int*ks=BL_LEAF_KEYS(pb);cl->k[cl->n]=ks[nk-1];cl->p[cl->n++]=cp;
PF_UnfixPage(fd,cp,TRUE);}
if(PF_AllocPage(fd,&pn,&pb)!=PFE_OK){fL(cl);return BL_ERROR;}
if(cp!=-1){char*prb;if(PF_GetThisPage(fd,cp,&prb,FALSE)==PFE_OK){
((BL_LeafHeader*)prb)->nextPage=pn;PF_UnfixPage(fd,cp,TRUE);}}
BL_LeafHeader*lh=(BL_LeafHeader*)pb;lh->pageType=BL_LEAF_PAGE;lh->numKeys=0;
lh->prevPage=cp;lh->nextPage=-1;if(fp==-1)fp=pn;cp=pn;nk=0;}
int*ks=BL_LEAF_KEYS(pb);int*vs=BL_LEAF_VALUES(pb);
ks[nk]=d[i].key;vs[nk++]=d[i].value;}
if(cp!=-1){BL_LeafHeader*lh=(BL_LeafHeader*)pb;lh->numKeys=nk;
eL(cl);int*ks=BL_LEAF_KEYS(pb);cl->k[cl->n]=ks[nk-1];cl->p[cl->n++]=cp;
PF_UnfixPage(fd,cp,TRUE);}
nl=1;while(cl->n>1){pl=cL();cp=-1;nk=0;for(i=0;i<cl->n;i++){
if(nk>=BL_MAX_INTERNAL_ENTRIES||cp==-1){if(cp!=-1){
BL_InternalHeader*ih=(BL_InternalHeader*)pb;ih->numKeys=nk;
eL(pl);int*ks=BL_INTERNAL_KEYS(pb);pl->k[pl->n]=ks[nk-1];pl->p[pl->n++]=cp;
PF_UnfixPage(fd,cp,TRUE);}
if(PF_AllocPage(fd,&pn,&pb)!=PFE_OK){fL(cl);fL(pl);return BL_ERROR;}
BL_InternalHeader*ih=(BL_InternalHeader*)pb;ih->pageType=BL_INTERNAL_PAGE;
ih->numKeys=0;ih->level=nl;cp=pn;nk=0;}
int*ks=BL_INTERNAL_KEYS(pb);int*ps=BL_INTERNAL_PTRS(pb);
ks[nk]=cl->k[i];ps[nk++]=cl->p[i];}
if(cp!=-1){BL_InternalHeader*ih=(BL_InternalHeader*)pb;ih->numKeys=nk;
eL(pl);int*ks=BL_INTERNAL_KEYS(pb);pl->k[pl->n]=ks[nk-1];pl->p[pl->n++]=cp;
PF_UnfixPage(fd,cp,TRUE);}
fL(cl);cl=pl;nl++;}
if(PF_GetThisPage(fd,0,&pb,FALSE)!=PFE_OK){fL(cl);return BL_ERROR;}
h=(BL_HeaderPage*)pb;h->rootPage=cl->p[0];h->firstLeaf=fp;
h->numLevels=nl;h->numRecords=nr;PF_UnfixPage(fd,0,TRUE);fL(cl);return BL_OK;}
