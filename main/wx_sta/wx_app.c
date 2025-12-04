#include "wx_app.h"
#include "wx_sta.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *WX_TAG = "wx_station";

static int WxGenDeviceNumber(uint8_t address, char *device_num, size_t size)
{
    const char *prefix = "ss20251130";
    return snprintf(device_num, size, "%s%04d", prefix, address);
}

static int WxSensorFrameAnalysis(const uint8_t *frame, WxStaDataFrame_t *out_data, size_t size)
{
    return 0;
}

static void WxUartQueueTask(void *arg)
{
    QueueHandle_t queue_handle = UartGetQueueHandle(2);
    QueueHandle_t wx_sta_data_handle = WxGetQueueHandle();
    uart_event_t event;
    WxStaDataFrame_t real_sensor_data = {0};
    uint8_t data[64] = {0};
    int flag = 0;
    while(true) {
        ESP_LOGI(WX_TAG, "Loop: %d", flag++);
        if (xQueueReceive(queue_handle, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case UART_DATA:
                    ESP_LOGI(WX_TAG, "UART_DATA Event: RX_DATA, Size: %d", event.size);
                    UartReceive(WX_STA_UART_NUM, data, event.size);
                    if (WxSensorFrameAnalysis(data, &real_sensor_data, event.size) != -1) {
                        xQueueSend(wx_sta_data_handle, &real_sensor_data, 0);
                        ESP_LOGI(WX_TAG, "Vaild frame.");
                        // print_sensor_data(&real_sensor_data);
                    }
                    for (int i = 0; i < event.size; i++) {
                        printf("%X ", data[i]);
                    }
                    printf("\n");
                    break;
                case UART_FIFO_OVF:
                    ESP_LOGW(WX_TAG, "FIFO over.");
                    break;
                default:
                    ESP_LOGI(WX_TAG, "uart event type: %d", event.type);
                    break;
            };
        }
    }
    vTaskDelete(NULL);
}

void NewWxUartEventHandleTask(void)
{
    xTaskCreate(WxUartQueueTask, "uart_task", 1024*5, NULL, 24, NULL);
}

