#include "wx_app.h"
#include "wx_sta.h"
#include "driver/uart.h"
#include "esp_log.h"

#define TIMEOUT_DEFAULT (20)

static const char *WX_TAG = "wx_station";
static WxStaDataFrame_t g_last_wx_frame;


static int WxSensorFrameAnalysis(const uint8_t *frame, WxStaDataFrame_t *out_data, size_t size)
{
    int index = 0;
    *out_data = g_last_wx_frame;
    for (int i = 0; i < size; i++) {
        if (frame[i]) {
            index = i;
            break;
        }
    }

    // request
    if (frame[index+1] ==  0x03) {
        // 使用地址区分传感器
        switch (frame[index]) {
        case SENSOR_TH:
            if (frame[index+2] != 0x04) {
                return -1;
            }

            int16_t temp = 0;
            uint16_t humi = 0;
            temp = (int16_t)(((uint16_t)frame[index+3] << 8) | frame[index+4]);
            humi = ((uint16_t)frame[index+5] << 8) | frame[index+6];

            out_data->temperature = (float)temp / 10.0f;
            out_data->humidity = (float)humi / 10.0f;
            out_data->flag |= FLAG_TH;
            break;
        case SENSOR_WS:
            if (frame[index+2] != 0x06) {
                return -1;
            }

            uint16_t speed1 = 0;
            uint16_t speed2 = 0;
            uint16_t level = 0;
            speed1 = ((uint16_t)frame[index+3] << 8) | frame[index+4];
            level = ((uint16_t)frame[index+5] << 8) | frame[index+6];
            speed2 = ((uint16_t)frame[index+7] << 8) | frame[index+8];

            out_data->wind_speed1 = (float)speed1 / 10.0f;
            out_data->wind_level = level;
            out_data->wind_speed2 = (float)speed2 / 100.0f;
            out_data->flag |= FLAG_WS;
            break;
        case SENSOR_WD:
            if (frame[index+2] != 0x04) {
                return -1;
            }

            uint16_t wind_dir = 0;
            uint16_t wind_angle = 0;
            wind_angle = ((uint16_t)frame[index+3] << 8) | frame[index+4];
            wind_dir = ((uint16_t)frame[index+5] << 8) | frame[index+6];

            out_data->wind_angle = (float)wind_angle / 10.0f;
            out_data->wind_dir = wind_dir;
            out_data->flag |= FLAG_WD;
            break;
        case SENSOR_SL:
            if (frame[index+2] != 0x02) {
                return -1;
            }

            uint16_t sunlight = 0;
            sunlight = ((uint16_t)frame[index+3] << 8) | frame[index+4];

            out_data->sunlight = sunlight * 10;
            out_data->flag |= FLAG_SL;
            break;
        case SENSOR_RG:
            if (frame[index+2] != 0x14) {
                return -1;
            }

            uint16_t rain_today;
            uint16_t rain_yest; 
            uint16_t rain_total;

            rain_today = ((uint16_t)frame[index+3] << 8) | frame[index+4];
            rain_yest = ((uint16_t)frame[index+7] << 8) | frame[index+8];
            rain_total = ((uint16_t)frame[index+9] << 8) | frame[index+10];

            out_data->rain_today = (float)rain_today / 10.0f;
            out_data->rain_yest = (float)rain_yest / 10.0f;
            out_data->rain_total = (float)rain_total / 10.0f;
            out_data->flag |= FLAG_RG;
            break;
        
        default:
            ESP_LOGI(WX_TAG, "Invaild Frame.");
            return -1;
            break;
        }
    }

    strncpy(out_data->index, "wx202512040001", DEVICE_NUMBER_SIZE);
    g_last_wx_frame = *out_data;
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
    int times = 0;
    while(true) {
        ESP_LOGI(WX_TAG, "Loop: %d", flag++);
        if (xQueueReceive(queue_handle, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case UART_DATA:
                    ESP_LOGI(WX_TAG, "UART_DATA Event: RX_DATA, Size: %d", event.size);
                    UartReceive(WX_STA_UART_NUM, data, event.size);
                    if (WxSensorFrameAnalysis(data, &real_sensor_data, event.size) != -1) {
                        ESP_LOGI(WX_TAG, "Vaild frame.");
                        if (real_sensor_data.flag == ALL_SENSORS_RECEIVED && times < TIMEOUT_DEFAULT) {
                            xQueueSend(wx_sta_data_handle, &real_sensor_data, 0);
                            times = 0;
                        } else {
                            times++;
                        } 
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

