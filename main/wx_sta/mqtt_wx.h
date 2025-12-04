#ifndef MQTT_WX_H
#define MQTT_WX_H

typedef struct {
    int time;
    char unit;
} TimeIntervalSetting_t;


#ifdef __cplusplus
extern "C" {
#endif

void MqttWxInit(void);
void NewWxMqttConnEventHandleTask(void);
void NewWxMqttSubQueueHandleTask(void);

#ifdef __cplusplus
}
#endif

#endif