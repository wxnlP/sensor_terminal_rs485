#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include "uart/uart.h"
#include <stdint.h>

// #define DEVICE_NUMBER_SIZE (15)
#define TRANSCEIVER_SOIL    (0x01)
#define TRANSCEIVER_321     (0xCD)
#define SENSOR_COMPLETE_EVENT_BIT   (1 << 0)

typedef struct {
    uint8_t index;         /* 传感器编号 */
    uint8_t battery;                        /* 电量 */
    int signal;                             /* 信号 */
    uint16_t suc_signal;                    /* 接收信号成功的次数 */
    uint16_t fail_signal;                   /* 接收信号失败的次数 */
    uint8_t signal_count;                   /* 信号计数器 */
    uint8_t vaild_count;                    /* 有效读值数量 */
    float t_soil;                           /* 温度 */
    float h_soil;                           /* 湿度 */
    float ec;                               /* 电导率 */
    float ph;                               /* PH值 */
    uint16_t n;                             /* 氮值 */
    uint16_t p;                             /* 磷值 */
    uint16_t k;                             /* 钾值 */
    float t_321;                            /* 温度 */
    float h_321;                            /* 湿度 */
    uint16_t sunlight;                      /* 光照 */
    uint8_t flag;
} SensorDataFrame_t;


typedef struct 
{
    uint8_t soil_count;
    uint8_t s321_count;
} SensorCounter_t;



#ifdef __cplusplus
extern "C" {
#endif

void TransceiverInit(void);
QueueHandle_t TransceiverGetSensorQueueHandle(void);
EventGroupHandle_t TransceiverGetCompleteHandle(void);
void NewUartEventHandleTask(void);
int TransceiverSensorDataRequest(uint8_t address, uint8_t sensor_num);


#ifdef __cplusplus
}
#endif
#endif