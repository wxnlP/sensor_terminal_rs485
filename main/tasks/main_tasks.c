#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi/wifi_ap.h"
#include "wifi/wifi_station.h"
#ifndef WX_STATION
#include "lora/mqtt_lora.h"
#else
#include "wx_sta/mqtt_wx.h"
#endif
#include "nvs_flash.h"
#include "esp_log.h"


static TaskHandle_t g_wifiStaHandle;
static void WifiStaTask(void *arg)
{
    (void) arg;
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    WifiStaInit();

    vTaskDelay(pdMS_TO_TICKS(1000));
    
#ifndef WX_STATION
    MqttLoraInit();
#else
    MqttWxInit();
#endif

    while (true)
    {
        WifiStaReconnect();
    }
    
}

static TaskHandle_t g_wifiApHandle;
static void WifiApTask(void *arg)
{
    (void) arg;
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    WifiApInit();

    while (true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
}


void NewWifiStaTask(void)
{
    xTaskCreate(WifiStaTask, "WifiStaTask", 3072, NULL, 24, &g_wifiStaHandle);
}

void NewWifiApTask(void)
{
    xTaskCreate(WifiApTask, "WifiApTask", 3072, NULL, 24, &g_wifiApHandle);
}