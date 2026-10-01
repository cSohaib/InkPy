#pragma once
#include <stdint.h>
#include "esp_err.h"
/* Serial-driven diagnostic, not reader UI. One owner; resources on microSD. */
esp_err_t ink_math_diagnostic_init(void);
unsigned ink_math_diagnostic_count(void);
void ink_math_diagnostic_draw(unsigned page, uint8_t *landscape_frame);
