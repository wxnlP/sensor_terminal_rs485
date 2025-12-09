#include "wx_app.h"
#include "driver/uart.h"
#include "esp_log.h"



#define TIMEOUT_DEFAULT (20)

static const char *WX_TAG = "wx_station";
static WxStaDataFrame_t g_last_wx_frame;
static EventGroupHandle_t g_wxScreenEventHandle;

/**
 * @brief 打印 WxStaDataFrame_t 结构体中的所有成员信息
 *
 * @param frame 指向要打印的 WxStaDataFrame_t 结构体指针
 */
void print_WxStaDataFrame(const WxStaDataFrame_t *frame)
{
    if (frame == NULL) {
        printf("Error: Frame pointer is NULL.\n");
        return;
    }

    printf("\n--- Weather Station Data Frame ---\n");
    
    // 基础信息
    printf("Index (Device ID):  %s\n", frame->index);
    printf("Flag (Status Byte): 0x%02X (%u)\n", frame->flag, frame->flag);

    // 气象数据
    printf("\n--- Environmental Metrics ---\n");
    printf("Temperature:        %.2f C\n", frame->temperature);
    printf("Humidity:           %.2f %%\n", frame->humidity);
    printf("Sunlight (Lux):     %u\n", frame->sunlight);

    // 风速风向
    printf("\n--- Wind Data ---\n");
    printf("Wind Direction (ID):%u\n", frame->wind_dir);
    printf("Wind Angle (Degree):%.2f deg\n", frame->wind_angle);
    printf("Wind Level:         %u\n", frame->wind_level);
    printf("Wind Speed (m/s):   %.2f m/s\n", frame->wind_speed1);
    printf("Wind Speed (km/h):  %.2f km/h\n", frame->wind_speed2);

    // 降雨量
    printf("\n--- Rain Data ---\n");
    printf("Rainfall Today:     %.2f mm\n", frame->rain_today);
    printf("Rainfall Yesterday: %.2f mm\n", frame->rain_yest);
    printf("Rainfall Total:     %.2f mm\n", frame->rain_total);

    printf("----------------------------------\n");
}


static int WxSensorFrameAnalysis(const uint8_t *frame, WxStaDataFrame_t *out_data, size_t size)
{
    int index = 0;

    for (int i = 0; i < size; i++) {
        if (frame[i]) {
            index = i;
            break;
        }
    }

    if (frame[index] == SENSOR_SCREEN) {
        return -1;
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
            // ESP_LOGI(WX_TAG, "Invaild Frame.");
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
                    // ESP_LOGI(WX_TAG, "UART_DATA Event: RX_DATA, Size: %d", event.size);
                    UartReceive(WX_STA_UART_NUM, data, event.size);
                    if (WxSensorFrameAnalysis(data, &real_sensor_data, event.size) != -1) {
                        // ESP_LOGI(WX_TAG, "Vaild frame.");
                        if (real_sensor_data.flag == ALL_SENSORS_RECEIVED || times > TIMEOUT_DEFAULT) {
                            print_WxStaDataFrame(&real_sensor_data);
                            vTaskDelay(pdMS_TO_TICKS(500));
                            xQueueSend(wx_sta_data_handle, &real_sensor_data, 0);
                            xEventGroupSetBits(g_wxScreenEventHandle, WX_SCREEN_EVENT);
                            times = 0;
                            real_sensor_data.flag = 0;
                        } else {
                            times++;
                        }
                        ESP_LOGI(WX_TAG, "frame times: %d", times);
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
    // 创建一个事件组
    g_wxScreenEventHandle = xEventGroupCreate();
    xTaskCreate(WxUartQueueTask, "uart_task", 1024*5, NULL, 24, NULL);
}

// 省事机构（快速完成功能需要，但不推荐使用，数据更新会落后一个帧）
void GetUartLastWxFrame(WxStaDataFrame_t *last_frame)
{
    *last_frame = g_last_wx_frame;
}

EventGroupHandle_t GetWxScreenEventHandle(void)
{
    return g_wxScreenEventHandle;
}

