#include "screen.h"
#include <stdio.h>
#include <string.h>
#include "uart/uart.h"

#define LED_UART_NUM    (2)

static const uint8_t GB_ALL_DATA[][6] = {
    {0xB1, 0xB1},                         // 北 (2 字节)
    {0xB1, 0xB1, 0xB6, 0xAB, 0xB1, 0xB1}, // 北东北 (6 字节)
    {0xB6, 0xAB, 0xB1, 0xB1},             // 东北 (4 字节)
    {0xB6, 0xAB, 0xB6, 0xAB, 0xB1, 0xB1}, // 东东北 (6 字节)
    {0xB6, 0xAB},                         // 东 (2 字节)
    {0xB6, 0xAB, 0xB6, 0xAB, 0xC4, 0xCF}, // 东东南 (6 字节)
    {0xB6, 0xAB, 0xC4, 0xCF},             // 东南 (4 字节)
    {0xC4, 0xCF, 0xB6, 0xAB, 0xC4, 0xCF}, // 南东南 (6 字节)
    {0xC4, 0xCF},                         // 南 (2 字节)
    {0xC4, 0xCF, 0xCE, 0xF7, 0xC4, 0xCF}, // 南西南 (6 字节)
    {0xCE, 0xF7, 0xC4, 0xCF},             // 西南 (4 字节)
    {0xCE, 0xF7, 0xCE, 0xF7, 0xC4, 0xCF}, // 西西南 (6 字节)
    {0xCE, 0xF7},                         // 西 (2 字节)
    {0xCE, 0xF7, 0xCE, 0xF7, 0xB1, 0xB1}, // 西西北 (6 字节)
    {0xCE, 0xF7, 0xB1, 0xB1},             // 西北 (4 字节)
    {0xB1, 0xB1, 0xCE, 0xF7, 0xB1, 0xB1}  // 北西北 (6 字节)
};


static uint16_t ModbusCRC16(const uint8_t *frame, uint8_t size) 
{
    uint16_t CRC = 0xFFFF;
    for (int i = 0; i < size; i++) {
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
    // printf("%X\n", CRC);
    return CRC;
}

int LEDSendFloat(float data, uint8_t index)
{
    FloatToBytes_t fb;
    fb.f = data;
    uint8_t frmae[13] = { 0x18,  0x10, 0x50, ((index+1)/2-1)*2 + 0x40, 0x00, 0x02, 0x04, 
                        fb.bytes[1], fb.bytes[0], fb.bytes[3], fb.bytes[2] };
    uint16_t crc = ModbusCRC16(frmae, 11);
    frmae[11] = (uint8_t)crc;
    frmae[12] = (uint8_t)(crc >> 8);

    // for (int i = 0; i < 13; i++) {
    //     printf("%02X ", frmae[i]);
    // }
    // printf("\n");
    // for (int i = 0; i < 4; i++) {
    //     printf("%02X ", fb.bytes[i]);
    // }
    return UartSend(LED_UART_NUM, frmae, 13);
}

int LEDSendUint16(uint16_t data, uint8_t index)
{

    uint8_t frmae[8] = { 0x18,  0x06, 0x10, (index-1), (uint8_t)(data>>8), (uint8_t)data};
    uint16_t crc = ModbusCRC16(frmae, 6);
    frmae[6] = (uint8_t)crc;
    frmae[7] = (uint8_t)(crc >> 8);

    // for (int i = 0; i < 8; i++) {
    //     printf("%02X ", frmae[i]);
    // }
    // printf("\n");
    return UartSend(LED_UART_NUM, frmae, 8);
}

int LEDSendString(const char *str, uint8_t index)
{
    uint8_t len = strlen(str);
    uint8_t byte_array[len];
    memcpy(byte_array, str, len);
    uint16_t s_addr = (index - 1) * 8;
    uint8_t frmae[20] = { 0x18, 0x10,  (uint8_t)(s_addr>>8), (uint8_t)s_addr,
                          0x00, (uint8_t)((len+1)/2) , len };
    for (int i = 0; i < len; i++) {
        frmae[i+7] = byte_array[i];
    }
    // printf("%X\n", len);
    uint16_t crc = ModbusCRC16(frmae, (9+len)-2);
    frmae[(9+len)-2] = (uint8_t)crc;
    frmae[(9+len)-1] = (uint8_t)(crc >> 8);

    // for (int i = 0; i < 9+len; i++) {
    //     printf("%02X ", frmae[i]);
    // }
    // printf("\n");

    return UartSend(LED_UART_NUM, frmae, 9+len);
}

int LEDSendChinese(uint8_t n, uint8_t index)
{
    uint16_t s_addr = (index - 1) * 8;
    uint8_t len = 0;
    uint8_t frmae[20] = { 0x18, 0x10,  (uint8_t)(s_addr>>8), (uint8_t)s_addr, 0x00};
    if (n == 0x00 || n == 0x04 || n == 0x08 || n == 0x0C) {
        frmae[5] = 0x01;
        frmae[6] = 0x02;
        frmae[7] = GB_ALL_DATA[n][0];
        frmae[8] = GB_ALL_DATA[n][1];
        uint16_t crc = ModbusCRC16(frmae, 9);
        frmae[9] = (uint8_t)crc;
        frmae[10] = (uint8_t)(crc >> 8);
        len = 11;
    } else if (n == 0x02 || n == 0x06 || n == 0x0A || n == 0x0E) {
        frmae[5] = 0x02;
        frmae[6] = 0x04;
        frmae[7] = GB_ALL_DATA[n][0];
        frmae[8] = GB_ALL_DATA[n][1];
        frmae[9] = GB_ALL_DATA[n][2];
        frmae[10] = GB_ALL_DATA[n][3];
        uint16_t crc = ModbusCRC16(frmae, 11);
        frmae[11] = (uint8_t)crc;
        frmae[12] = (uint8_t)(crc >> 8);
        len = 13;
    } else if (n == 0x01 || n == 0x03 || n == 0x05 || n == 0x07 || n == 0x09 || n == 0x0B || n == 0x0D || n == 0x0F) {
        frmae[5] = 0x03;
        frmae[6] = 0x06;
        frmae[7] = GB_ALL_DATA[n][0];
        frmae[8] = GB_ALL_DATA[n][1];
        frmae[9] = GB_ALL_DATA[n][2];
        frmae[10] = GB_ALL_DATA[n][3];
        frmae[11] = GB_ALL_DATA[n][4];
        frmae[12] = GB_ALL_DATA[n][5];
        uint16_t crc = ModbusCRC16(frmae, 13);
        frmae[13] = (uint8_t)crc;
        frmae[14] = (uint8_t)(crc >> 8);
        len = 15;
    }
    // for (int i = 0; i < len; i++) {
    //     printf("%02X ", frmae[i]);
    // }
    // printf("\n");
    return UartSend(LED_UART_NUM, frmae, len);
}