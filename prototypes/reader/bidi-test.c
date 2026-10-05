#include "ink_bidi.h"
#include <assert.h>
#include <stdlib.h>
int main(void)
{
    const uint32_t alphabet[]={'A','b',' ',1,'1','2','(',')',0x627,0x628,0x644,0x64e,0x651,0x200c,0x200d,0x202b,0x202c,0x2066,0x2069,0xfffc};
    uint32_t logical[128];ink_bidi shaped;
    assert(ink_bidi_shape(logical,0,&shaped)<0);
    for(unsigned trial=0;trial<500;trial++) {
        unsigned n=1+(unsigned)rand()%128;
        for(unsigned i=0;i<n;i++)logical[i]=alphabet[(unsigned)rand()%(sizeof(alphabet)/sizeof(*alphabet))];
        assert(!ink_bidi_shape(logical,n,&shaped));bool seen[128]={0};
        for(unsigned i=0;i<n;i++) {assert(shaped.map[i]>=0&&shaped.map[i]<(int)n&&!seen[shaped.map[i]]);seen[shaped.map[i]]=true;}
    }
    return 0;
}
