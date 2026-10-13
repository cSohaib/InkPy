#pragma once
#include "ink_browser.h"
#include "ink_editor.h"
#include "ink_console.h"
#include "ink_power.h"
/* Fixed-size device checkpoint; no VM, task or live stream is saved. */
typedef struct {
    ink_browser browser;
    ink_editor editor; /* work is NULL on disk; temporary owns the draft. */
    ink_console console;
    ink_power power;
    bool editing,reading,python,landscape;
    unsigned document,page,reader_view,chapter_first;
    char digits[11];
} ink_resume;
int ink_resume_save(const ink_resume *state,const uint8_t frame[48000]);
int ink_resume_load(ink_resume *state,uint8_t frame[48000]);
