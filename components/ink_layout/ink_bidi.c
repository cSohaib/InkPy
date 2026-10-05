#include "ink_bidi.h"
#include "fribidi.h"
#include <stdlib.h>
bool ink_bidi_arabic(uint32_t cp)
{ return fribidi_get_bidi_type(cp)==FRIBIDI_TYPE_AL; }
bool ink_bidi_mark(uint32_t cp)
{ return fribidi_get_bidi_type(cp)==FRIBIDI_TYPE_NSM; }
bool ink_bidi_hidden(uint32_t cp)
{
    FriBidiCharType t=fribidi_get_bidi_type(cp);
    return FRIBIDI_IS_EXPLICIT(t)||t==FRIBIDI_TYPE_BN||
        cp==0x200e||cp==0x200f||cp==0x061c||(cp>=0x2066&&cp<=0x2069);
}
bool ink_bidi_word(uint32_t cp)
{
    FriBidiCharType t=fribidi_get_bidi_type(cp);
    return t==FRIBIDI_TYPE_LTR||t==FRIBIDI_TYPE_RTL||t==FRIBIDI_TYPE_AL||
        t==FRIBIDI_TYPE_EN||t==FRIBIDI_TYPE_AN||t==FRIBIDI_TYPE_NSM||cp=='_'||cp=='\''||cp=='-';
}
int ink_bidi_shape(const uint32_t *logical,unsigned count,ink_bidi *out)
{
    if(!count||count>INK_BIDI_CELLS)return -1;
    typedef struct {
        FriBidiCharType types[INK_BIDI_CELLS];
        FriBidiBracketType brackets[INK_BIDI_CELLS];
        FriBidiLevel levels[INK_BIDI_CELLS];
        FriBidiArabicProp joins[INK_BIDI_CELLS];
    } Scratch;
    Scratch *s=calloc(1,sizeof(*s));if(!s)return -1;
    fribidi_get_bidi_types(logical,(int)count,s->types);
    fribidi_get_bracket_types(logical,(int)count,s->types,s->brackets);
    FriBidiParType base=FRIBIDI_PAR_LTR;
    for(unsigned i=0;i<count;i++) {
        out->visual[i]=logical[i];out->map[i]=(int)i;
    }
    /* Per displayed line, not per document or paragraph. Numbers/punctuation
     * do not choose direction. Non-Arabic strong letters default to LTR. */
    for(unsigned i=0;i<count;i++) {
        if(s->types[i]==FRIBIDI_TYPE_AL){base=FRIBIDI_PAR_RTL;break;}
        if(s->types[i]==FRIBIDI_TYPE_LTR||s->types[i]==FRIBIDI_TYPE_RTL)break;
    }
    out->rtl=base==FRIBIDI_PAR_RTL;
    int result=-1;
    if(!fribidi_get_par_embedding_levels_ex(s->types,s->brackets,(int)count,&base,s->levels))goto done;
    fribidi_get_joining_types(logical,(int)count,s->joins);
    fribidi_join_arabic(s->types,(int)count,s->levels,s->joins);
    FriBidiFlags flags=FRIBIDI_FLAG_SHAPE_MIRRORING|FRIBIDI_FLAG_REORDER_NSM|FRIBIDI_FLAGS_ARABIC;
    fribidi_shape(flags,s->levels,(int)count,s->joins,out->visual);
    if(fribidi_reorder_line(flags,s->types,(int)count,0,base,s->levels,out->visual,out->map))result=0;
done:
    free(s);return result;
}
