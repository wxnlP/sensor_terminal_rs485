#ifndef MQTT_LORA_H
#define MQTT_LORA_H


typedef struct {
    int time;
    char unit;
} TimeIntervalSetting_t;

#ifdef __cplusplus
extern "C" {
#endif

void MqttLoraInit(void);
int GetTimeInterval(void);
void NewMqttConnEventHandleTask(void);
void NewMqttSubQueueHandleTask(void);


#ifdef __cplusplus
}
#endif

#endif