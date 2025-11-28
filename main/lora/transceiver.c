#include "transceiver.h"
#include "driver/uart.h"
#include "uart/uart.h"
#include "esp_log.h"
#include <stdint.h>


static const char *LORA_TAG = "transceiver";

static void Uart2QueueTask(void *arg)
{
    QueueHandle_t queue_handle = UartGetQueueHandle(2);
    uint8_t data[64] = {0};
    uart_event_t event;
    int flag = 0;
    while(true) {
        ESP_LOGI(LORA_TAG, "Loop: %d", flag++);
        if (xQueueReceive(queue_handle, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case UART_DATA:
                    ESP_LOGI(LORA_TAG, "Event: RX_DATA, Size: %d", event.size);
                    UartReceive(2, data, event.size);
                    // UartSend(2, data, event.size);
                    ESP_LOGI(LORA_TAG, "%s", (char *)data);
                    break;
                case UART_FIFO_OVF:
                    ESP_LOGW(LORA_TAG, "FIFO over.");
                    break;
                default:
                    ESP_LOGI(LORA_TAG, "uart event type: %d", event.type);
                    break;
            };
        }
    }
    vTaskDelete(NULL);
}

void UartEventHandleTasks(void)
{
    xTaskCreate(Uart2QueueTask, "uart_task", 1024*5, NULL, 24, NULL);
}