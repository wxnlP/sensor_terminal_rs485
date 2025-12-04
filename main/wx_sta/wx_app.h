#ifndef WX_APP_H
#define WX_APP_H

typedef enum {
    SENSOR_TH = 0x01,   /* 温湿度 */
    SENSOR_WS,          /* 风速 */
    SENSOR_WD,          /* 风向 */
    SENSOR_SL,          /* 光照强度 */
    SENSOR_RG,          /* 雨量 */
} SensorIndex_t;

#ifdef __cplusplus
extern "C" {
#endif

void NewWxUartEventHandleTask(void);

#ifdef __cplusplus
}
#endif

#endif