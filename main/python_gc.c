/* Adapted from MicroPython ports/esp32/gccollect.c, pinned revision in
 * docs/MICROPYTHON.md. Copyright (c) 2014 Damien P. George, 2017 Pycom Limited. MIT licence retained
 * in third_party/notices/MicroPython-LICENSE.txt and generated embed sources. */
#include "py/mpstate.h"
#include "py/gc.h"
#include "esp_cpu.h"
#include "xtensa/hal.h"

/* Preserve the upstream volatile recursion: it spills all Xtensa register
 * windows before tracing roots on the single VM owner's internal task stack. */
static void collect_inner(volatile unsigned level)
{
    if(level<XCHAL_NUM_AREGS/8) collect_inner(level+1);
    else {
        volatile uint32_t sp=(uint32_t)esp_cpu_get_sp();
        gc_collect_root((void **)sp,((mp_uint_t)MP_STATE_THREAD(stack_top)-sp)/sizeof(uint32_t));
    }
}
void gc_helper_collect_regs_and_stack(void) { collect_inner(0); }
