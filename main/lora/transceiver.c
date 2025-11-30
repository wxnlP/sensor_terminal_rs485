#include "transceiver.h"
#include "driver/uart.h"
#include "esp_log.h"


#define LORA_UART_NUM (2)

static QueueHandle_t g_sensorDataHandle;
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
    ESP_LOGI(TAG, "Sensor ID: %u, Battery: %u%%", 
             data->index, data->battery);
    
    ESP_LOGI(TAG, "Signal: %d dBm, Valid Count: %u, Signal Counter: %u", 
             data->signal, data->vaild_count, data->signal_count);

    // --- 打印通信统计 ---
    ESP_LOGI(TAG, "Suc/Fail Signals: %u / %u", 
             data->suc_signal, data->fail_signal);

    // --- 打印测量值 (使用 %.2f 格式化浮点数，保留两位小数) ---
    ESP_LOGI(TAG, "Temperature: %.2f C, Humidity: %.2f %%", 
             (double)data->temperature, (double)data->humidity);

    ESP_LOGI(TAG, "EC: %.2f, PH: %.2f", 
             (double)data->ec, (double)data->ph);

    // --- 打印 NPK 值 ---
    ESP_LOGI(TAG, "NPK (N/P/K): %u / %u / %u", 
             data->n, data->p, data->k);
    
    ESP_LOGI(TAG, "==================================================");
}

static int TransceiverGenerateDeviceNumber(uint8_t address, char *device_num, size_t size)
{
    const char *prefix = "ss20251130";
    return snprintf(device_num, size, "%s%04d", prefix, address);
}

static int TransceiverSensorFrameAnalysis(const uint8_t *frame, SensorDataFrame_t *out_data)
{
    
    // out_data->index = frame[0];
    TransceiverGenerateDeviceNumber(frame[0], out_data->index, 15);
    out_data->battery = frame[3];
    out_data->signal = -(frame[4] / 2);
    out_data->suc_signal = (frame[5] << 8) | frame[6];
    out_data->fail_signal = (frame[7] << 8) | frame[8];
    out_data->signal_count = frame[9];
    out_data->vaild_count = frame[10];
    out_data->temperature = ((frame[11] << 8) | frame[12]) / 10.0f - 30.0f;
    out_data->humidity = ((frame[13] << 8) | frame[14]) / 10.0f;
    out_data->ec = (frame[15] << 8) | frame[16];
    out_data->ph = ((frame[17] << 8) | frame[18]) / 10.0f;
    out_data->n = (frame[19] << 8) | frame[20];
    out_data->p = (frame[21] << 8) | frame[22];
    out_data->k = (frame[23] << 8) | frame[24];
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
                    TransceiverSensorFrameAnalysis(data, &real_sensor_data);
                    xQueueSend(g_sensorDataHandle, &real_sensor_data, 0);
                    print_sensor_data(&real_sensor_data);
                    // for (int i = 0; i < event.size; i++) {
                    //     printf("%X ", data[i]);
                    // }
                    // printf("\n");
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
    g_sensorDataHandle = xQueueCreate(10, sizeof(SensorDataFrame_t));
}

QueueHandle_t TransceiverGetSensorQueueHandle(void)
{
    return g_sensorDataHandle;
}

int TransceiverSensorDataRequest(uint8_t address)
{
    uint16_t s_address = 0x10 * address;
    uint8_t frame[8] = { address, 0x03,  (uint8_t)s_address >> 8, (uint8_t)s_address,
                         0x00, 0x10, 0, 0};
    uint16_t crc = ModbusCRC16(frame);
    frame[6] = (uint8_t)(crc & 0x00FF);
    frame[7] = (uint8_t)(crc >> 8);

    return UartSend(LORA_UART_NUM, frame, 8);
}