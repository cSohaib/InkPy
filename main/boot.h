#pragma once
#include "esp_err.h"
void ink_boot_report(void);
/* Call only after board/display/frame initialization and first refresh succeed. */
esp_err_t ink_boot_confirm(void);
