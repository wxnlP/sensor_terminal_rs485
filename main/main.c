#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "gpio/led.h"
#include "uart/uart.h"
#include "lora/transceiver.h"


#define UART_SEND_DEMO      (0)
#define UART_RECEIVE_DEMO   (0)

static const char *MainTag = "main";

void app_main(void)
{
    LedInit();
    UartInit(2, 115200);
    RS485Init(2);
    UartEventHandleTasks();
    int value = 0;
    uint8_t buffer[32] = {0};
    char rev_buffer[16] = {0};
   
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
        vTaskDelay(pdMS_TO_TICKS(1000));
        LedCtrl(1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        #endif
    }
}