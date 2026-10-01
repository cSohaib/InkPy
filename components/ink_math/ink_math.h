#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Experimental shared host/ESP32 renderer. One owner, initialize once per process. All storage owned
 * by caller except library internals. 480x800 portrait, row-major, 1=black.
 * Firmware's existing diagnostic uses landscape, 1=white: not interchangeable. */
enum { INK_MATH_WIDTH = 480, INK_MATH_HEIGHT = 800, INK_MATH_BYTES = 48000 };
typedef struct {
    int width, height, baseline; /* bitmap includes 8px padding on each side */
    char error[160];            /* on failure caller should show literal source */
} ink_math_result;
int ink_math_init(const char *resources, char *error, size_t error_size);
int ink_math_render(const char *source, int display, int pixels,
                    uint8_t bitmap[INK_MATH_BYTES], ink_math_result *result);
void ink_math_shutdown(void);
#ifdef __cplusplus
}
#endif

