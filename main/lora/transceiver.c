#include "transceiver.h"
#include "driver/uart.h"
#include "esp_log.h"


#define LORA_UART_NUM (2)

static QueueHandle_t g_sensorDataHandle;
static EventGroupHandle_t g_sensor_complete_handle;
static SensorCounter_t g_sensor_count = {0};
static const char *LORA_TAG = "transceiver";
static const char *TAG = "DEBUG";

static void print_sensor_data(const SensorDataFrame_t *data)
{
    if (data == NULL) {
        ESP_LOGE(TAG, "Sensor data is NULL.");
        return;
    }

    // --- 打印基本信息和状态 ---
    ESP_LOGI(TAG, "================ Sensor Data Report ================");
    ESP_LOGI(TAG, "Sensor ID: %u, Battery: %u%%, Flag: %d", 
             data->index, data->battery, data->flag);
    
    ESP_LOGI(TAG, "Signal: %d dBm, Valid Count: %u, Signal Counter: %u", 
             data->signal, data->vaild_count, data->signal_count);

    // --- 打印通信统计 ---
    ESP_LOGI(TAG, "Suc/Fail Signals: %u / %u", 
             data->suc_signal, data->fail_signal);

    // --- 打印测量值 (使用 %.2f 格式化浮点数，保留两位小数) ---
    ESP_LOGI(TAG, "T_soil: %.2f C, H_soil: %.2f %%", 
             (double)data->t_soil, (double)data->t_soil);

    ESP_LOGI(TAG, "EC: %.2f, PH: %.2f", 
             (double)data->ec, (double)data->ph);

    // --- 打印 NPK 值 ---
    ESP_LOGI(TAG, "NPK (N/P/K): %u / %u / %u", 
             data->n, data->p, data->k);
    ESP_LOGI(TAG, "T_321: %.2f C, H_321: %.2f %%, Sunlight: %d", 
             (double)data->t_321, (double)data->h_321, data->sunlight);
    
    ESP_LOGI(TAG, "==================================================");
}


static int TransceiverSensorFrameAnalysis(const uint8_t *frame, SensorDataFrame_t *out_data, size_t size)
{
    int index = 0;
    for (int i = 0; i < size; i++) {
        if (frame[i]) {
            index = i;
            break;
        }
    }

    if (frame[index+2] != 0x20) {
        return -1;
    }

    if (frame[index+3] == 0x00) {
        return -1;
    }

    if (frame[index] == TRANSCEIVER_SOIL) {
        /* 土壤传感器 */
        out_data->index = g_sensor_count.soil_count;
        out_data->battery = frame[index + 3];
        out_data->signal = -(frame[index + 4] / 2);
        out_data->suc_signal = (frame[index + 5] << 8) | frame[index + 6];
        out_data->fail_signal = (frame[index + 7] << 8) | frame[index + 8];
        out_data->signal_count = frame[index + 9];
        out_data->vaild_count = frame[index + 10];
        out_data->t_soil = ((frame[index + 11] << 8) | frame[index + 12]) / 10.0f - 30.0f;
        out_data->h_soil = ((frame[index + 13] << 8) | frame[index + 14]) / 10.0f;
        out_data->ec = (frame[index + 15] << 8) | frame[index + 16];
        out_data->ph = ((frame[index + 17] << 8) | frame[index + 18]) / 10.0f;
        out_data->n = (frame[index + 19] << 8) | frame[index + 20];
        out_data->p = (frame[index + 21] << 8) | frame[index + 22];
        out_data->k = (frame[index + 23] << 8) | frame[index + 24];
        out_data->flag = (1 << 0);
    } else if (frame[index] == TRANSCEIVER_321) {
        /* 三合一传感器 */
        out_data->index = g_sensor_count.s321_count;
        out_data->battery = frame[index + 3];
        out_data->t_321 = ((frame[index + 11] << 8) | frame[index + 12]) / 10.0f - 40.0f;
        out_data->h_321 = ((frame[index + 13] << 8) | frame[index + 14]) / 10.0f;
        out_data->sunlight = ((frame[index + 15] << 8) | frame[index + 16]) * 4;
        out_data->flag = (1 << 1);
    }
    return 0;
}

static void Uart2QueueTask(void *arg)
{
    QueueHandle_t queue_handle = UartGetQueueHandle(2);
    uart_event_t event;
    SensorDataFrame_t real_sensor_data = {0};
    uint8_t data[64] = {0};
    int flag = 0;
    while(true) {
        ESP_LOGI(LORA_TAG, "Loop: %d", flag++);
        if (xQueueReceive(queue_handle, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case UART_DATA:
                    ESP_LOGI(LORA_TAG, "UART_DATA Event: RX_DATA, Size: %d", event.size);
                    UartReceive(LORA_UART_NUM, data, event.size);
                    int ret = TransceiverSensorFrameAnalysis(data, &real_sensor_data, event.size);
                    if ( ret != -1) {
                        xQueueSend(g_sensorDataHandle, &real_sensor_data, 0);
                        print_sensor_data(&real_sensor_data);
                        memset(&real_sensor_data, 0, sizeof(real_sensor_data));
                    }
                    // 发送标志位，收到数据并处理完毕
                    xEventGroupSetBits(g_sensor_complete_handle, SENSOR_COMPLETE_EVENT_BIT);
                    for (int i = 0; i < event.size; i++) {
                        printf("%X ", data[i]);
                    }
                    printf("\n");
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

void NewUartEventHandleTask(void)
{
    xTaskCreate(Uart2QueueTask, "uart_task", 1024*5, NULL, 24, NULL);
}


static uint16_t ModbusCRC16(const uint8_t *frame) 
{
    uint16_t CRC = 0xFFFF;
    for (int i = 0; i < 6; i++) {
        CRC ^= frame[i];
        for (int j = 0; j < 8; j++) {
            if (CRC & 0x0001) {
                CRC >>= 1;
                CRC ^= 0xA001;
            } else {
                CRC >>= 1;
            }
        }
    }
    return CRC;
}

void TransceiverInit(void)
{
    UartInit(2, 115200);
    RS485Init(2);
    g_sensorDataHandle = xQueueCreate(40, sizeof(SensorDataFrame_t));
    g_sensor_complete_handle = xEventGroupCreate();
}

QueueHandle_t TransceiverGetSensorQueueHandle(void)
{
    return g_sensorDataHandle;
}

EventGroupHandle_t TransceiverGetCompleteHandle(void)
{
    return g_sensor_complete_handle;
}

int TransceiverSensorDataRequest(uint8_t address, uint8_t sensor_num)
{
    if (address == 0x01) {
        g_sensor_count.soil_count = sensor_num;
    } else if (address == 0xCD) {
        g_sensor_count.s321_count = sensor_num;
    }
    uint16_t s_address = 0x10 * sensor_num;
    // ESP_LOGW(TAG, "%d", s_address);
    uint8_t frame[8] = { address, 0x03,  (uint8_t)(s_address >> 8), (uint8_t)s_address,
                         0x00, 0x10, 0, 0};
    uint16_t crc = ModbusCRC16(frame);
    frame[6] = (uint8_t)(crc & 0x00FF);
    frame[7] = (uint8_t)(crc >> 8);

    return UartSend(LORA_UART_NUM, frame, 8);
}