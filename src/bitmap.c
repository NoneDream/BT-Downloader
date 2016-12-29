#include <stdlib.h>
#include "bitmap.h"

bitmap *bitmap_init(int len){
    bitmap *p;
    int l=len/sizeof(int);

    p=(bitmap *)malloc(sizeof(bitmap));
    p->len=len;
    p->bmap=(int *)malloc(l);

    for(int *a=p->bmap,int i=0;i<l;++i)p->bmap[i]=0;

    return p;
}

void bitmap_free(bitmap &(*in)){
    free(in->bmap);
    free(in);
    in=NULL;
}

int bitmap_set(bitmap *in,unsigned int n){
    int k=n/sizeof(int);
    int y=n%sizeof(int);

    if(n>=in->len)return -1;

    in->bmap[k]=in->bmap[k]|(1<<y);
    return 0;
}

int bitmap_unset(bitmap *in,unsigned int n){
    int k=n/sizeof(int);
    int y=n%sizeof(int);

    if(n>=in->len)return -1;

    in->bmap[k]=in->bmap[k]&(~(1<<y));
    return 0;
}

int bitmap_test(bitmap *in,unsigned int n){
    int k=n/sizeof(int);
    int y=n%sizeof(int);

    if(n>=in->len)return -1;

    if(in->bmap[k]&(1<<y)==0)return 0;
    else return 1;
}
