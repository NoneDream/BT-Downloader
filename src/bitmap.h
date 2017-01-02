#ifndef __BITMAP__
#define __BITMAP__

typedef struct _bitmap{
    unsigned int len;
    int *bmap;
}bitmap;

bitmap *bitmap_init(int len);

void bitmap_free(bitmap &(*in));

int bitmap_set(bitmap *in,unsigned int n);

int bitmap_unset(bitmap *in,unsigned int n);

int bitmap_test(bitmap *in,unsigned int n);

#endif
