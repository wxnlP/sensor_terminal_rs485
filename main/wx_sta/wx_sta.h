#ifndef WX_STA_H
#define WX_STA_H

#include "uart/uart.h"
#include <stdint.h>

#define WX_STA_UART_NUM (2)
#define DEVICE_NUMBER_SIZE (15)

// 定义标志位宏，用于设置和检查
#define FLAG_TH (1 << 0)
#define FLAG_WS (1 << 1)
#define FLAG_WD (1 << 2)
#define FLAG_SL (1 << 3)
#define FLAG_RG (1 << 4)

#define ALL_SENSORS_RECEIVED (FLAG_TH | FLAG_WS | FLAG_WD | FLAG_SL | FLAG_RG)

typedef struct {
    char index[DEVICE_NUMBER_SIZE];
    float temperature;      /* 温度 */
    float humidity;         /* 湿度 */
    uint16_t wind_dir;      /* 风向方位 */
    float wind_angle;       /* 风向方位角度 */
    uint16_t wind_level;    /* 风级 */
    float wind_speed1;      /* 风速m/s */
    float wind_speed2;      /* 风速km/h */
    uint16_t sunlight;      /* 光照强度 */
    float rain_today;       /* 今日降雨量 */
    float rain_yest;        /* 昨日降雨量 */
    float rain_total;       /* 总降雨量 */
    uint8_t flag;
} WxStaDataFrame_t;

typedef enum {
    RATE_4800 = 0,
    RATE_9600,
    RATE_19200,
    RATE_57600,
    RATE_115200,
} Buadrate_t;

typedef enum {
    WIND_DIR_NORTH          = 0x00, // 北 (RS485: 0X00)
    WIND_DIR_N_NORTHEAST    = 0x01, // 北东北 (RS485: 0X01)
    WIND_DIR_NORTHEAST      = 0x02, // 东北 (RS485: 0X02)
    WIND_DIR_E_NORTHEAST    = 0x03, // 东东北 (RS485: 0X03)
    WIND_DIR_EAST           = 0x04, // 东 (RS485: 0X04)
    WIND_DIR_E_SOUTHEAST    = 0x05, // 东东南 (RS485: 0X05)
    WIND_DIR_SOUTHEAST      = 0x06, // 东南 (RS485: 0X06)
    WIND_DIR_S_SOUTHEAST    = 0x07, // 南东南 (RS485: 0X07)
    WIND_DIR_SOUTH          = 0x08, // 南 (RS485: 0X08)
    WIND_DIR_S_SOUTHWEST    = 0x09, // 南西南 (RS485: 0X09)
    WIND_DIR_SOUTHWEST      = 0x0A, // 西南 (RS485: 0X0A)
    WIND_DIR_W_SOUTHWEST    = 0x0B, // 西西南 (RS485: 0X0B)
    WIND_DIR_WEST           = 0x0C, // 西 (RS485: 0X0C)
    WIND_DIR_W_NORTHWEST    = 0x0D, // 西西北 (RS485: 0X0D)
    WIND_DIR_NORTHWEST      = 0x0E, // 西北 (RS485: 0X0E)
    WIND_DIR_N_NORTHWEST    = 0x0F, // 北西北 (RS485: 0X0F)
    
    // 添加一个无效值用于错误处理
    WIND_DIR_INVALID        = 0xFF
} WindDirection_t;

typedef enum {
    AutoHeatDisable = 0,
    AutoHeatEnable,
} AutoHeat_t;


#ifdef __cplusplus
extern "C" {
#endif

void WxInit(int baud_rate);
QueueHandle_t WxGetQueueHandle(void);
int WxSetAddress(uint8_t old_address, uint8_t new_address);
int WxSetBaudrate(uint8_t address, Buadrate_t rate);
int WxWindDirDataRequest(uint8_t address);
int WxWindSpeedDataRequest(uint8_t address);
int WxWindHeatEnable(uint8_t address, AutoHeat_t auto_heat);
int WxWindSetHeatStartTh(uint8_t address, int temp);
int WxWindSetHeatStopTh(uint8_t address, int temp);
int WxWindHeatThRequest(uint8_t address);
int WxTHDataRequest(uint8_t address);
int WxSunlightDataRequest(uint8_t address);
int WxRainTimeDataRequest(uint8_t address);
int WxRainDataRequest(uint8_t address);
int WxRainDataResetRequest(uint8_t address);

#ifdef __cplusplus
}
#endif

#endif