#include "math_diagnostic.h"
#include "ink_math.h"
#include "math_cases.h"
#include "pins.h"
#include <inttypes.h>
#include <string.h>
#include <stdatomic.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "math-probe";
static uint8_t *bitmap;
static atomic_uint allocation_failures;
static atomic_uint last_failed_bytes;
static int64_t phase_start;

static void allocation_failed(size_t bytes, uint32_t caps, const char *function)
{
    (void)caps; (void)function;
    /* No allocation, logging or locks inside the allocator callback. */
    atomic_fetch_add_explicit(&allocation_failures, 1, memory_order_relaxed);
    atomic_store_explicit(&last_failed_bytes, (unsigned)bytes, memory_order_relaxed);
}

static void phase_begin(void)
{
    ESP_ERROR_CHECK(heap_caps_monitor_local_minimum_free_size_start());
    phase_start = esp_timer_get_time();
}

static void phase_end(const char *phase)
{
    int64_t elapsed = esp_timer_get_time() - phase_start;
    const uint32_t caps[] = {MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT,
                             MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT};
    for (unsigned i = 0; i < 2; ++i) {
        ESP_LOGI(TAG, "%s %s free=%u local_min_free=%u largest=%u", phase,
                 i ? "PSRAM" : "internal", (unsigned)heap_caps_get_free_size(caps[i]),
                 (unsigned)heap_caps_get_minimum_free_size(caps[i]),
                 (unsigned)heap_caps_get_largest_free_block(caps[i]));
    }
    ESP_ERROR_CHECK(heap_caps_monitor_local_minimum_free_size_stop());
    ESP_LOGI(TAG, "%s elapsed_us=%" PRId64 " stack_min_free_bytes=%u alloc_failures=%u last_failed_bytes=%u",
             phase, elapsed, (unsigned)uxTaskGetStackHighWaterMark(NULL),
             allocation_failures, (unsigned)last_failed_bytes);
}

esp_err_t ink_math_diagnostic_init(void)
{
    if (!heap_caps_get_total_size(MALLOC_CAP_SPIRAM)) return ESP_ERR_NO_MEM;
    esp_err_t hook = heap_caps_register_failed_alloc_callback(allocation_failed);
    if (hook != ESP_OK) return hook;
    phase_begin();
    bitmap = heap_caps_malloc(INK_MATH_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    char error[160] = {0};
    int result = bitmap ? ink_math_init("/sd/inkpy/math", error, sizeof(error)) : -1;
    phase_end("init");
    if (result != 0) {
        ESP_LOGE(TAG, "math init failed: %s", bitmap ? error : "bitmap allocation");
        heap_caps_free(bitmap); bitmap = NULL;
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "%u cases, 24px, Prev/Next pages; last three intentionally show an X", ink_math_diagnostic_count());
    return ESP_OK;
}

unsigned ink_math_diagnostic_count(void)
{
    return sizeof(math_cases) / sizeof(math_cases[0]);
}

void ink_math_diagnostic_draw(unsigned page, uint8_t *frame)
{
    const math_case_t *sample = &math_cases[page % ink_math_diagnostic_count()];
    ink_math_result result;
    phase_begin();
    int status = ink_math_render(sample->source, sample->display, 24, bitmap, &result);
    phase_end("render");
    ESP_LOGI(TAG, "case=%u status=%s expected=%s dimensions=%dx%d baseline=%d source=%s",
             page + 1, status ? "fallback" : "rendered", sample->fallback ? "fallback" : "rendered",
             result.width, result.height, result.baseline, sample->source);
    if ((status != 0) != sample->fallback) ESP_LOGE(TAG, "UNEXPECTED RESULT");
    if (status) {
        ESP_LOGW(TAG, "fallback: %s", result.error);
        /* No reader error UI yet: an X indicates fallback, details stay serial. */
        memset(bitmap, 0, INK_MATH_BYTES);
        for (int i = 0; i < 40; ++i) {
            int x = 20 + i, y = 20 + i;
            bitmap[y * 60 + x / 8] |= 0x80 >> (x % 8);
            x = 59 - i;
            bitmap[y * 60 + x / 8] |= 0x80 >> (x % 8);
        }
    }
    memset(frame, 0xff, PANEL_BYTES);
    /* Portrait black=1 -> clockwise native landscape white=1. No redraw on wake. */
    for (int y = 0; y < INK_MATH_HEIGHT; ++y)
        for (int x = 0; x < INK_MATH_WIDTH; ++x)
            if (bitmap[y * 60 + x / 8] & (0x80 >> (x % 8))) {
                int dx = INK_MATH_HEIGHT - 1 - y, dy = x;
                frame[dy * PANEL_STRIDE + dx / 8] &= (uint8_t)~(0x80 >> (dx % 8));
            }
}
