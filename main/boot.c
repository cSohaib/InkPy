#include "boot.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "sdkconfig.h"

#ifdef CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
#error "InkPy boot confirmation must not enable eFuse anti-rollback updates"
#endif
static const char *TAG="boot";
void ink_boot_report(void)
{
    ESP_LOGI(TAG,"reset reason=%d (esp_reset_reason_t)",(int)esp_reset_reason());
    const esp_partition_t *running=esp_ota_get_running_partition();
    if(!running) { ESP_LOGW(TAG,"running partition unavailable"); return; }
    esp_ota_img_states_t state=ESP_OTA_IMG_UNDEFINED;
    esp_err_t e=esp_ota_get_state_partition(running,&state);
    ESP_LOGI(TAG,"running=%s offset=0x%lx size=0x%lx OTA state=%d query=%s",
             running->label,(unsigned long)running->address,
             (unsigned long)running->size,(int)state,esp_err_to_name(e));
}
esp_err_t ink_boot_confirm(void)
{
    const esp_partition_t *running=esp_ota_get_running_partition();
    if(!running) return ESP_ERR_NOT_FOUND;
    if(running->subtype==ESP_PARTITION_SUBTYPE_APP_FACTORY) return ESP_OK;
    esp_ota_img_states_t state;
    esp_err_t e=esp_ota_get_state_partition(running,&state);
    if(e!=ESP_OK) return e;
    if(state!=ESP_OTA_IMG_NEW && state!=ESP_OTA_IMG_PENDING_VERIFY) return ESP_OK;
    /* Works with the existing rollback-enabled bootloader even when our build's
     * generated bootloader disables rollback. Never install that bootloader. */
    e=esp_ota_mark_app_valid_cancel_rollback();
    ESP_LOGI(TAG,"first screen succeeded; confirm %s: %s",running->label,esp_err_to_name(e));
    return e;
}
