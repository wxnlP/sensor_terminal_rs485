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
    ESP_LOGI(MainTag, "##########\nWX Station\n##########");
    WxInit(9600);
#endif
    /* New freertos tasks */
    NewWifiStaTask();
#ifndef WX_STATION  
    NewUartEventHandleTask();
    // vTaskDelay(pdMS_TO_TICKS(1000));
    NewMqttConnEventHandleTask();
    NewMqttSubQueueHandleTask();
#else
    NewWxUartEventHandleTask();
    NewWxMqttConnEventHandleTask();
    NewWxMqttSubQueueHandleTask();
    NewWxScreenTask();
#endif

    // int value = 0;
    // uint8_t buffer[32] = {0};
    // char rev_buffer[16] = {0};
   
    while(true)
    {
#if UART_SEND_DEMO == 1
        snprintf((char *)buffer, sizeof(buffer),"[INFO] AT+LOCK=%d\r\n", value);
        value++;
        UartSend(2, buffer, sizeof(buffer));
        vTaskDelay(pdMS_TO_TICKS(200));
        LedCtrl(0);
        UartSendString(2, "ON\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
        LedCtrl(1);
        UartSendString(2, "OFF\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));        
#endif

#if UART_RECEIVE_DEMO == 1
        if (UartRevString(2, rev_buffer, 6) > 0) {
            ESP_LOGI(MainTag, "Rec:%s", rev_buffer);
            UartSendString(2, rev_buffer);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
#endif

#if UART_RECEIVE_DEMO != 1 && UART_SEND_DEMO != 1
        LedCtrl(0);
        vTaskDelay(pdMS_TO_TICKS(5000));
        LedCtrl(1);
        vTaskDelay(pdMS_TO_TICKS(5000));
#ifndef WX_STATION
        TransceiverSensorDataRequest(1);
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

#endif
    }
}