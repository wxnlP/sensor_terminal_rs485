#include "uart.h"

#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"


#define UART0_TX_PIN (1)
#define UART0_RX_PIN (3)
#define UART1_TX_PIN (10)
#define UART1_RX_PIN (9)
#define UART2_TX_PIN (5)
#define UART2_RX_PIN (4)

#define UART_RX_BUFFER_SIZE (1024 * 2)
#define UART_TX_BUFFER_SIZE (1024 * 2)
#define EVENT_QUEUE_SIZE (10)
#define RS485_DR_PIN (21)

static uint8_t g_rs485_flag = 0;
static QueueHandle_t g_uart_queue_handle[3];
// static const char *UART_TAG = "uart";


void UartInit(uint8_t uart_num, int baud_rate)
{
    uart_driver_install(uart_num, UART_RX_BUFFER_SIZE, UART_TX_BUFFER_SIZE,
                        EVENT_QUEUE_SIZE, &g_uart_queue_handle[uart_num], 0);

    uart_config_t config = {
        // 波特率
        .baud_rate = baud_rate,
        // 数据位
        .data_bits = UART_DATA_8_BITS,
        // 停止位
        .stop_bits = UART_STOP_BITS_1,
        // 硬件流控(信号线来管理数据的发送和接收。)
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        // 校验位
        .parity = UART_PARITY_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(uart_num, &config);

    switch (uart_num) {
        case 0:
            uart_set_pin(uart_num, UART0_TX_PIN, UART0_RX_PIN, 
                         UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
            break;
        case 1:
            uart_set_pin(uart_num, UART1_TX_PIN, UART1_RX_PIN, 
                         UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
            break;
        case 2:
            uart_set_pin(uart_num, UART2_TX_PIN, UART2_RX_PIN, 
                         UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
            break;
        default:
            break;
    };
}

void RS485Init(uint8_t uart_num)
{
    gpio_reset_pin(RS485_DR_PIN);
    gpio_set_direction(RS485_DR_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(RS485_DR_PIN, 0);
    g_rs485_flag |= (1 << (uart_num + 1));
}

void RS485SendEnable(void)
{
    gpio_set_level(RS485_DR_PIN, 1);
}

void RS485ReceiveEnable(void)
{
    gpio_set_level(RS485_DR_PIN, 0);
}

int UartSend(uint8_t uart_num, const uint8_t *data, uint32_t len)
{
    if (g_rs485_flag == (1 << (uart_num + 1))) {
        gpio_set_level(RS485_DR_PIN, 1);
    }
    
    int ret = uart_write_bytes(uart_num, (const uint8_t *)data, (size_t)len);
    
    if (g_rs485_flag == (1 << (uart_num + 1))) {
        uart_wait_tx_done(uart_num, portMAX_DELAY);
        gpio_set_level(RS485_DR_PIN, 0);
    }

    return ret;
}

int UartSendString(uint8_t uart_num,const char *str)
{
    if (g_rs485_flag == (1 << (uart_num + 1))) {
        gpio_set_level(RS485_DR_PIN, 1);
    }  
    
    int ret = uart_write_bytes(uart_num, (const char *)str, strlen(str));
    if (g_rs485_flag == (1 << (uart_num + 1))) {
        uart_wait_tx_done(uart_num, portMAX_DELAY);
        gpio_set_level(RS485_DR_PIN, 0);
    }
    return ret;
}

int UartReceive(uint8_t uart_num, uint8_t *data, uint32_t len)
{
    if (g_rs485_flag == (1 << (uart_num + 1))) {
        gpio_set_level(RS485_DR_PIN, 0);
    }  
    return uart_read_bytes(uart_num, (uint8_t *)data, len, portMAX_DELAY);
}

int UartRevString(uint8_t uart_num, char *data, uint32_t len)
{
    if (g_rs485_flag == (1 << (uart_num + 1))) {
        gpio_set_level(RS485_DR_PIN, 0);
    }
    return uart_read_bytes(uart_num, (char *)data, len, portMAX_DELAY);
}

QueueHandle_t UartGetQueueHandle(uint8_t uart_num)
{
    return g_uart_queue_handle[uart_num];
}



