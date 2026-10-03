#include "ink_console.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    ink_console c; ink_console_init(&c); c.busy=true;
    assert(!ink_console_key(&c,'x') && !c.used);
    c.waiting=true;
    assert(!ink_console_key(&c,'4'));
    assert(!ink_console_key(&c,'3'));
    assert(!ink_console_key(&c,INK_KEY_DELETE));
    assert(!ink_console_key(&c,'2'));
    assert(ink_console_key(&c,INK_KEY_ENTER));
    assert(!strcmp(c.input,"42") && c.busy && c.waiting);
    ink_console_completed(&c,false);
    assert(!c.waiting && !c.busy && !c.used);
    c.busy=true; c.waiting=true;
    assert(ink_console_key(&c,INK_KEY_ENTER));
    assert(!c.used); puts("PASS: input keys, delete, Enter, empty line and completion");
}
