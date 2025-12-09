#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi/wifi_ap.h"
#include "wifi/wifi_station.h"
#ifndef WX_STATION
#include "lora/mqtt_lora.h"
#else
#include "wx_sta/mqtt_wx.h"
#include "wx_sta/wx_app.h"
#include "screen/screen.h"
#endif
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>


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

#ifdef WX_STATION
static TaskHandle_t g_screenHandle;
static void WxScreenTask(void *arg)
{
    WxStaDataFrame_t wx_frame;
    char str[6];
    EventGroupHandle_t wx_screen_handle = GetWxScreenEventHandle();
    while (true)
    {
        EventBits_t uxBits = xEventGroupWaitBits(wx_screen_handle,
                                                  WX_SCREEN_EVENT, 
                                                  pdFALSE, 
                                                  pdTRUE,
                                                  portMAX_DELAY);
        if (uxBits & WX_SCREEN_EVENT) {
            GetUartLastWxFrame(&wx_frame);
            LEDSendFloat(wx_frame.temperature, INDEX_TEMPERATURE);
            vTaskDelay(pdMS_TO_TICKS(2000));
            LEDSendFloat(wx_frame.humidity, INDEX_HUMIDITY);
            vTaskDelay(pdMS_TO_TICKS(2000));
            LEDSendChinese(wx_frame.wind_dir, INDEX_WIND_DIR);
            vTaskDelay(pdMS_TO_TICKS(2000));
            LEDSendFloat(wx_frame.wind_angle, INDEX_WIND_ANGLE);
            vTaskDelay(pdMS_TO_TICKS(2000));
            snprintf(str, sizeof(str), "%d", wx_frame.wind_level);
            LEDSendString(str, INDEX_WIND_LEVEL);
            vTaskDelay(pdMS_TO_TICKS(2000));
            LEDSendFloat(wx_frame.wind_speed1, INDEX_WIND_SPEED);
            vTaskDelay(pdMS_TO_TICKS(2000));
            snprintf(str, sizeof(str), "%d", wx_frame.sunlight);
            LEDSendString(str, INDEX_SUNLIGHT);
            vTaskDelay(pdMS_TO_TICKS(2000));
            LEDSendFloat(wx_frame.rain_total, INDEX_RAIN_TOTAL);
            vTaskDelay(pdMS_TO_TICKS(2000));
            LEDSendFloat(wx_frame.rain_today, INDEX_RAIN_TODAY);
            vTaskDelay(pdMS_TO_TICKS(2000));
            LEDSendFloat(wx_frame.rain_yest, INDEX_RAIN_YEST);
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}
#endif

void NewWifiStaTask(void)
{
    xTaskCreate(WifiStaTask, "WifiStaTask", 3072, NULL, 24, &g_wifiStaHandle);
}

void NewWifiApTask(void)
{
    xTaskCreate(WifiApTask, "WifiApTask", 3072, NULL, 24, &g_wifiApHandle);
}

#ifdef WX_STATION
void NewWxScreenTask(void)
{
    xTaskCreate(WxScreenTask, "WxScreenTask", 3072, NULL, 24, &g_screenHandle);
}
#endif