#include "bl.h"
#include "blinternal.h"
static int bs(int*k,int n,int s,int*f){
int l=0,r=n-1,m;*f=0;while(l<=r){m=(l+r)/2;if(k[m]==s){*f=1;return m;}
else if(k[m]<s)l=m+1;else r=m-1;}return l;}
int BL_Search(int fd,int k,int*v){
char*pb;BL_HeaderPage*h;int cp,e,f,p,pt;
if(PF_GetThisPage(fd,0,&pb,FALSE)!=PFE_OK)return BL_ERROR;
h=(BL_HeaderPage*)pb;cp=h->rootPage;if(cp==-1){PF_UnfixPage(fd,0,FALSE);return BL_KEYNOTFOUND;}
PF_UnfixPage(fd,0,FALSE);while(1){
if(PF_GetThisPage(fd,cp,&pb,FALSE)!=PFE_OK)return BL_ERROR;
pt=*((int*)pb);if(pt==BL_LEAF_PAGE){
BL_LeafHeader*lh=(BL_LeafHeader*)pb;int*ks=BL_LEAF_KEYS(pb);int*vs=BL_LEAF_VALUES(pb);
p=bs(ks,lh->numKeys,k,&f);if(f){*v=vs[p];PF_UnfixPage(fd,cp,FALSE);return BL_OK;}
else{PF_UnfixPage(fd,cp,FALSE);return BL_KEYNOTFOUND;}}else{
BL_InternalHeader*ih=(BL_InternalHeader*)pb;int*ks=BL_INTERNAL_KEYS(pb);int*ps=BL_INTERNAL_PTRS(pb);
p=bs(ks,ih->numKeys,k,&f);if(f||p>=ih->numKeys)p=(p>=ih->numKeys)?ih->numKeys-1:p;
int np=ps[p];PF_UnfixPage(fd,cp,FALSE);cp=np;}}}
