#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

#define INDEX_TEMPERATURE   (1)
#define INDEX_HUMIDITY      (3)
#define INDEX_WIND_DIR      (2)
#define INDEX_WIND_ANGLE    (7)
#define INDEX_WIND_LEVEL    (4)
#define INDEX_WIND_SPEED    (11)
#define INDEX_SUNLIGHT      (13)
#define INDEX_RAIN_TOTAL    (15)
#define INDEX_RAIN_TODAY    (17)
#define INDEX_RAIN_YEST     (19)

typedef union {
    float f;
    uint8_t bytes[4];
} FloatToBytes_t;

#ifdef __cplusplus
extern "C" {
#endif

int LEDSendFloat(float data, uint8_t index);
int LEDSendUint16(uint16_t data, uint8_t index);
int LEDSendString(const char *str, uint8_t index);
int LEDSendChinese(uint8_t n, uint8_t index);

#ifdef __cplusplus
}
#endif

#endif

