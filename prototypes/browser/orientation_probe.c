#include "orientation.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    uint8_t original[PANEL_BYTES],rotated[PANEL_BYTES];
    for(unsigned i=0;i<PANEL_BYTES;i++) original[i]=(uint8_t)(i*37+i/13);
    memcpy(rotated,original,sizeof(rotated)); ink_frame_rotate_180(rotated);
    for(unsigned y=0;y<PANEL_HEIGHT;y++) for(unsigned x=0;x<PANEL_WIDTH;x++) {
        unsigned tx=PANEL_WIDTH-1-x,ty=PANEL_HEIGHT-1-y;
        assert(((original[y*PANEL_STRIDE+x/8]>>(7-x%8))&1)==
               ((rotated[ty*PANEL_STRIDE+tx/8]>>(7-tx%8))&1));
        unsigned ux,uy; ink_panel_to_ui(tx,ty,&ux,&uy);
        /* Original host drawing: panel_x=799-ui_y, panel_y=ui_x. */
        assert(ux==y && uy==PANEL_WIDTH-1-x);
    }
    ink_frame_rotate_180(rotated); assert(!memcmp(original,rotated,sizeof(rotated)));
    puts("PASS: all 384000 pixels rotate 180 degrees; touch coordinates agree; two rotations restore every byte");
}
