#include <port/mpconfigport_common.h>
void ink_python_poll(void);
#define MICROPY_VM_HOOK_LOOP ink_python_poll();
#define MICROPY_ENABLE_VM_ABORT (1)
#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_CORE_FEATURES)
#define MICROPY_ENABLE_COMPILER (1)
#define MICROPY_ENABLE_GC (1)
#define MICROPY_PY_GC (1)
#define MICROPY_HELPER_REPL (1)
#define MICROPY_STACK_CHECK (1)
#define MICROPY_FLOAT_IMPL (MICROPY_FLOAT_IMPL_DOUBLE)
#define MICROPY_LONGINT_IMPL (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_ENABLE_EXTERNAL_IMPORT (0)
#define MICROPY_PY_SYS (1)
#define MICROPY_PY_SYS_PLATFORM "inkpy"
#define MICROPY_PY_IO (0)
/* Embed's helper header has no Xtensa register type. Actual collection uses
 * python_gc.c's upstream ESP32 register-window spilling strategy, not setjmp. */
#define MICROPY_GCREGS_SETJMP (1)
