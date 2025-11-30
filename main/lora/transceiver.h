#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include "uart/uart.h"
#include <stdint.h>

#define DEVICE_NUMBER_SIZE (15)

typedef struct {
    char index[DEVICE_NUMBER_SIZE];         /* 传感器编号 */
    uint8_t battery;                        /* 电量 */
    int signal;                             /* 信号 */
    uint16_t suc_signal;                    /* 接收信号成功的次数 */
    uint16_t fail_signal;                   /* 接收信号失败的次数 */
    uint8_t signal_count;                   /* 信号计数器 */
    uint8_t vaild_count;                    /* 有效读值数量 */
    float temperature;                      /* 温度 */
    float humidity;                         /* 湿度 */
    float ec;                               /* 电导率 */
    float ph;                               /* PH值 */
    uint16_t n;                             /* 氮值 */
    uint16_t p;                             /* 磷值 */
    uint16_t k;                             /* 钾值 */
} SensorDataFrame_t;

#ifdef __cplusplus
extern "C" {
#endif

void TransceiverInit(void);
QueueHandle_t TransceiverGetSensorQueueHandle(void);
void NewUartEventHandleTask(void);
int TransceiverSensorDataRequest(uint8_t address);

#ifdef __cplusplus
}
#endif
#endif