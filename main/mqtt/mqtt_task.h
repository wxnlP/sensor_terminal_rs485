#ifndef MQTT_TASK_H
#define MQTT_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"


#define MQTT_CONNECTED_BIT      (1<<0)
#define MQTT_DISCONNECTED_BIT   (1<<1)
#define MAX_TOPIC_LEN    64
#define MAX_DATA_LEN     (128 * 4)

typedef struct {
    char topic[MAX_TOPIC_LEN];
    int topic_len;
    char data[MAX_DATA_LEN];
    int data_len;
} MqttSubData_t;

#ifdef __cplusplus
extern "C" {
#endif

void MqttInit(const char* uri, const char* user_name, const char* passwd, const char* client_id);
EventGroupHandle_t MqttGetConnEventHandle(void);
QueueHandle_t MqttGetSubQueueHandle(void);
int MqttClientPublish(const char* topic, const char *data, int qos);
int MqttClientSubscribeSingle(const char* topic, int qos);

#ifdef __cplusplus
}
#endif

#endif