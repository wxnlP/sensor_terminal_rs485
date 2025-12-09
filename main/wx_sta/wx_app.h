#ifndef WX_APP_H
#define WX_APP_H

#include "wx_sta.h"
#include "freertos/event_groups.h"

#define WX_SCREEN_EVENT (1 << 0)

typedef enum {
    SENSOR_TH = 0x01,   /* 温湿度 */
    SENSOR_WS,          /* 风速 */
    SENSOR_WD,          /* 风向 */
    SENSOR_SL,          /* 光照强度 */
    SENSOR_RG,          /* 雨量 */
    SENSOR_SCREEN = 0x18
} SensorIndex_t;

#ifdef __cplusplus
extern "C" {
#endif

void NewWxUartEventHandleTask(void);
void GetUartLastWxFrame(WxStaDataFrame_t *last_frame);
EventGroupHandle_t GetWxScreenEventHandle(void);

#ifdef __cplusplus
}
#endif

#endif