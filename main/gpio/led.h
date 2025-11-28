#ifndef LED_H
#define LED_H


#include <stdint-gcc.h>


#define GPIO_LED_PIN    2


#ifdef __cplusplus
extern "C" {
#endif

void LedInit(void);
void LedCtrl(uint32_t level);


#ifdef __cplusplus
}
#endif

#endif