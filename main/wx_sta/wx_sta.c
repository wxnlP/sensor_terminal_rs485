#include "wx_sta.h"


#define WX_STA_UART_NUM (2)

static QueueHandle_t g_wxStaDataHandle;
// static const char *WX_TAG = "wx_station";
static const uint16_t BAUDRATE[5] = { 0x4800, 0x9600, 0x1920, 0x5760, 0x1152 };

static uint16_t ModbusCRC16(const uint8_t *frame) 
{
    uint16_t CRC = 0xFFFF;
    for (int i = 0; i < 6; i++) {
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
    return CRC;
}

void WxInit(int baud_rate)
{
    UartInit(WX_STA_UART_NUM, baud_rate);
    RS485Init(WX_STA_UART_NUM);
    g_wxStaDataHandle = xQueueCreate(10, sizeof(WxStaDataFrame_t));
}

int WxSetAddress(uint8_t old_address, uint8_t new_address)
{
    uint8_t frame[8];
    frame[0] = old_address;
    frame[1] = 0x06;
    frame[2] = 0x00;
    frame[3] = 0x30;
    frame[4] = 0x00;
    frame[5] = new_address;
    uint16_t crc = ModbusCRC16(frame);
    frame[6] = (uint8_t)(crc & 0x00FF);
    frame[7] = (uint8_t)(crc >> 8);
    
    return UartSend(WX_STA_UART_NUM, frame, 8);
}

int WxSetBaudrate(uint8_t address, Buadrate_t rate)
{
    uint8_t frame[8];
    frame[0] = address;
    frame[1] = 0x06;
    frame[2] = 0x00;
    frame[3] = 0x31;
    frame[4] = (uint8_t)(BAUDRATE[rate] >> 8);
    frame[5] = (uint8_t)(BAUDRATE[rate] & 0x00FF);
    uint16_t crc = ModbusCRC16(frame);
    frame[6] = (uint8_t)(crc & 0x00FF);
    frame[7] = (uint8_t)(crc >> 8);

    return UartSend(WX_STA_UART_NUM, frame, 8);
}

int WxWindDataRequest(uint8_t address)
{
    uint8_t frame[8];
    frame[0] = address;
    frame[1] = 0x03;
    frame[2] = 0x00;
    frame[3] = 0x00;
    frame[4] = 0x00;
    frame[5] = 0x02;
    uint16_t crc = ModbusCRC16(frame);
    frame[6] = (uint8_t)(crc & 0x00FF);
    frame[7] = (uint8_t)(crc >> 8);

    return UartSend(WX_STA_UART_NUM, frame, 8);
}

int WxWindHeatEnable(uint8_t address, AutoHeat_t auto_heat)
{
    uint8_t frame[8];
    frame[0] = address;
    frame[1] = 0x06;
    frame[2] = 0x00;
    frame[3] = 0x6B;
    frame[4] = 0x00;
    frame[5] = (uint8_t)auto_heat;
    uint16_t crc = ModbusCRC16(frame);
    frame[6] = (uint8_t)(crc & 0x00FF);
    frame[7] = (uint8_t)(crc >> 8);

    return UartSend(WX_STA_UART_NUM, frame, 8);
}

int WxWindSetHeatStartTh(uint8_t address, int temp)
{
    uint8_t frame[8];
    uint16_t t = (uint16_t)(temp * 10);
    frame[0] = address;
    frame[1] = 0x06;
    frame[2] = 0x00;
    frame[3] = 0x6C;
    frame[4] = (uint8_t)(t >> 8);
    frame[5] = (uint8_t)(t & 0x00FF);
    uint16_t crc = ModbusCRC16(frame);
    frame[6] = (uint8_t)(crc & 0x00FF);
    frame[7] = (uint8_t)(crc >> 8);

    return UartSend(WX_STA_UART_NUM, frame, 8);
}

int WxWindSetHeatStopTh(uint8_t address, int temp)
{
    uint8_t frame[8];
    uint16_t t = (uint16_t)(temp * 10);
    frame[0] = address;
    frame[1] = 0x06;
    frame[2] = 0x00;
    frame[3] = 0x6D;
    frame[4] = (uint8_t)(t >> 8);
    frame[5] = (uint8_t)(t & 0x00FF);
    uint16_t crc = ModbusCRC16(frame);
    frame[6] = (uint8_t)(crc & 0x00FF);
    frame[7] = (uint8_t)(crc >> 8);

    return UartSend(WX_STA_UART_NUM, frame, 8);
}

int WxWindHeatThRequest(uint8_t address)
{
    uint8_t frame[8];
    frame[0] = address;
    frame[1] = 0x03;
    frame[2] = 0x00;
    frame[3] = 0x6C;
    frame[4] = 0x00;
    frame[5] = 0x02;
    uint16_t crc = ModbusCRC16(frame);
    frame[6] = (uint8_t)(crc & 0x00FF);
    frame[7] = (uint8_t)(crc >> 8);

    return UartSend(WX_STA_UART_NUM, frame, 8);
}

int WxTHDataRequest(uint8_t address)
{
    uint8_t frame[8];
    frame[0] = address;
    frame[1] = 0x03;
    frame[2] = 0x00;
    frame[3] = 0x00;
    frame[4] = 0x00;
    frame[5] = 0x02;
    uint16_t crc = ModbusCRC16(frame);
    frame[6] = (uint8_t)(crc & 0x00FF);
    frame[7] = (uint8_t)(crc >> 8);

    return UartSend(WX_STA_UART_NUM, frame, 8);
}
