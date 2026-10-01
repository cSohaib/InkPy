#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "esp_err.h"

typedef struct {
    bool home;
    uint8_t contacts;
    uint16_t x, y; /* Native landscape coordinates. */
} ink_touch_t;

/* Single hardware-owner task in this diagnostic. No concurrent bus callers. */
esp_err_t ink_board_init(void);
esp_err_t ink_touch_enable(bool enabled);
esp_err_t ink_touch_read(ink_touch_t *out, bool *fresh);
esp_err_t ink_light_set(unsigned brightness, unsigned warmth, bool on);
esp_err_t ink_light_sleep(bool sleeping);
esp_err_t ink_rtc_read(struct tm *out);
esp_err_t ink_sd_mount(void);
esp_err_t ink_sd_probe(void); /* Exclusive temporary file; never replaces a user file. */
void ink_board_report(void);

typedef enum { INK_SSD1677, INK_UC8179, INK_UC8279 } ink_panel_t;
esp_err_t ink_display_init(void);
ink_panel_t ink_display_panel(void);
esp_err_t ink_display_frame(const uint8_t frame[48000]);
esp_err_t ink_display_sleep(void); /* Preserve physical pixels; next draw reinitializes RAM. */

/* Blocks the diagnostic task in state-preserving light sleep until a long Power
 * hold. Future Python integration MUST quiesce VM/storage/network first. */
esp_err_t ink_sleep(void);
