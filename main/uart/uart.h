#ifndef UART_H
#define UART_H

#include <stdint-gcc.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

void UartInit(uint8_t uart_num, int baud_rate);
void RS485Init(uint8_t uart_num);
void RS485SendEnable(void);
void RS485ReceiveEnable(void);
int UartSend(uint8_t uart_num, const uint8_t *data, uint32_t len);
int UartSendString(uint8_t uart_num, const char *str);
int UartReceive(uint8_t uart_num, uint8_t *data, uint32_t len);
int UartRevString(uint8_t uart_num, char *data, uint32_t len);
// void UartEventHandleTasks(void);
QueueHandle_t UartGetQueueHandle(uint8_t uart_num);


#ifdef __cplusplus
}
#endif

#endif