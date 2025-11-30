#ifndef MQTT_LORA_H
#define MQTT_LORA_H


typedef struct {
    int time;
    char unit[2];
} TimeIntervalSetting_t;

#ifdef __cplusplus
extern "C" {
#endif

void MqttLoraInit(void);
void NewMqttConnEventHandleTask(void);
void NewMqttSubQueueHandleTask(void);


#ifdef __cplusplus
}
#endif

#endif