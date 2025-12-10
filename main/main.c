#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "gpio/led.h"
#ifndef WX_STATION 
#include "lora/transceiver.h"
#include "lora/mqtt_lora.h"
#else
#include "wx_sta/wx_sta.h"
#include "wx_sta/wx_app.h"
#include "wx_sta/mqtt_wx.h"
#endif
#include "tasks/main_tasks.h"


#define UART_SEND_DEMO      (0)
#define UART_RECEIVE_DEMO   (0)

static const char *MainTag = "main";

void app_main(void)
{
    LedInit();
#ifndef WX_STATION 
    TransceiverInit();
#else
    WxInit(9600);
#endif
    /* New freertos tasks */
    NewWifiStaTask();
#ifndef WX_STATION  
    NewUartEventHandleTask();
    NewMqttConnEventHandleTask();
    NewMqttSubQueueHandleTask();
#else
    NewWxUartEventHandleTask();
    NewWxMqttConnEventHandleTask();
    NewWxMqttSubQueueHandleTask();
    NewWxScreenTask();
#endif

    EventGroupHandle_t sensor_complete_handle =  TransceiverGetCompleteHandle();
    while(true)
    {
        int delay_time = GetTimeInterval();
        LedCtrl(0);
        vTaskDelay(pdMS_TO_TICKS(delay_time/2));
        LedCtrl(1);
        vTaskDelay(pdMS_TO_TICKS(delay_time/2));
#ifndef WX_STATION
        for (int i = 1; i < 33; i++) {
            TransceiverSensorDataRequest(TRANSCEIVER_SOIL, i);
            xEventGroupWaitBits(sensor_complete_handle, 
                                SENSOR_COMPLETE_EVENT_BIT, 
                                pdTRUE, 
                                pdTRUE, 
                                pdMS_TO_TICKS(1000));
        }
        for (int j = 1; j < 33; j++) {
            TransceiverSensorDataRequest(TRANSCEIVER_321, j);
            xEventGroupWaitBits(sensor_complete_handle, 
                                SENSOR_COMPLETE_EVENT_BIT, 
                                pdTRUE, 
                                pdTRUE, 
                                pdMS_TO_TICKS(1000));
        }
        
        
#else
        // 温湿度
        WxTHDataRequest(0x01);
        ESP_LOGI(MainTag, "温湿度");
        vTaskDelay(pdMS_TO_TICKS(1000));
        // 风速
        WxWindSpeedDataRequest(0x02);
        ESP_LOGI(MainTag, "风速");
        vTaskDelay(pdMS_TO_TICKS(1000));
        // 风向
        WxWindDirDataRequest(0x03);
        ESP_LOGI(MainTag, "风向");
        vTaskDelay(pdMS_TO_TICKS(1000));
        // 光照强度
        WxSunlightDataRequest(0x04);
        ESP_LOGI(MainTag, "光照");
        vTaskDelay(pdMS_TO_TICKS(1000));
        // 降雨量
        WxRainDataRequest(0x05);
        ESP_LOGI(MainTag, "降雨量");
        vTaskDelay(pdMS_TO_TICKS(1000));
#endif

    }
}