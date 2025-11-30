#include "mqtt_lora.h"
#include "cJSON.h"
#include "esp_log.h"
#include "lora/transceiver.h"
#include "mqtt/mqtt_task.h"
#include <string.h>

#ifdef HUAWEI_CLOUD
#define MQTT_URI        "mqtt://9dfa5c258d.st1.iotda-device.cn-east-3.myhuaweicloud.com:1883"
#define MQTT_USERNAME   "677388cfbab900244b135588_DATAS"
#define MQTT_PASSWD     "ad31747e0b97a6ca1287043ca46a3571796f0d20e1056ecabec4f05fc2db545f"
#define MQTT_CLIENT_ID  "677388cfbab900244b135588_DATAS_0_0_2024123106"
#define MQTT_DEVICE_ID  "677388cfbab900244b135588_DATAS"
#define MQTT_SERVICE_ID "soil_sensor"
// HUAWEICLOUDE平台的话题定义
#define  MQTT_PublishTopic      "$oc/devices/677388cfbab900244b135588_DATAS/sys/properties/report"
#define  MQTT_RequestTopic      "$oc/devices/677388cfbab900244b135588_DATAS/sys/shadow/get/request_id={request_id}"
#define  MQTT_SubscribeTopic    "$oc/devices/677388cfbab900244b135588_DATAS/sys/shadow/get/response/#"
#else
#define MQTT_URI                "mqtt://u2f8813a.ala.dedicated.huawei.emqxcloud.cn:1883"
#define MQTT_USERNAME           "soilsensor"
#define MQTT_PASSWD             "admin123"
#define MQTT_CLIENT_ID          "frame"
#define MQTT_SERVICE_ID         "soil_sensor"
#define MQTT_PUBLLISH_TOPPIC    "soil/sensor"
#define MQTT_SUBSCRIBE_TOPIC    "soil/time"
#endif

#define S (1000)
#define M (60 * S)
#define H (60 * M)
#define DEFAULT_UPLOAD_TIME_INTERVAL (10 * S)

static uint64_t g_uploadTimeInterval;
static const char *MQTT_LORA = "mqtt_lora";


static char *MqttSoilSensorPublishPayload(SensorDataFrame_t *frame)
{
    // 创建JSON结构
    cJSON *root = cJSON_CreateObject();
    cJSON *services = cJSON_CreateArray();

    // 构建服务节点
    cJSON *upload = cJSON_CreateObject();
    cJSON_AddStringToObject(upload, "service_id", MQTT_SERVICE_ID);

    // 添加属性
    cJSON *props = cJSON_CreateObject();

    cJSON_AddStringToObject(props, "index", frame->index);
    cJSON_AddNumberToObject(props, "battery", frame->battery);
    cJSON_AddNumberToObject(props, "signal", frame->signal);
    cJSON_AddNumberToObject(props, "suc_signal", frame->suc_signal);
    cJSON_AddNumberToObject(props, "fail_signal", frame->fail_signal);
    cJSON_AddNumberToObject(props, "signal_count", frame->signal_count);
    cJSON_AddNumberToObject(props, "vaild_count", frame->vaild_count);
    cJSON_AddNumberToObject(props, "signal_count", frame->signal_count);
    cJSON_AddNumberToObject(props, "temperature", frame->temperature);
    cJSON_AddNumberToObject(props, "humidity", frame->humidity);
    cJSON_AddNumberToObject(props, "ec", frame->ec);
    cJSON_AddNumberToObject(props, "ph", frame->ph);
    cJSON_AddNumberToObject(props, "n", frame->n);
    cJSON_AddNumberToObject(props, "p", frame->p);
    cJSON_AddNumberToObject(props, "k", frame->k);

    cJSON_AddItemToObject(upload, "properties", props);

    // 组合结构
    cJSON_AddItemToArray(services, upload);
    cJSON_AddItemToObject(root, "services", services);

    // 生成字符串
    char *payload = cJSON_PrintUnformatted(root);

    // 释放cJSON树（注意：不释放payload字符串）
    cJSON_Delete(root);

    return payload;
}

static int MqttAnalysisSubPyload(const char *json_str, TimeIntervalSetting_t *ti)
{
    cJSON *root = cJSON_Parse(json_str);
    cJSON *value_item = cJSON_GetObjectItemCaseSensitive(root, "time");
    if (!cJSON_IsNumber(value_item)) {
        cJSON_Delete(root);
        return -1;
    }
    ti->time = (int)value_item->valuedouble;
    cJSON *unit_item = cJSON_GetObjectItemCaseSensitive(root, "unit");
    if (!cJSON_IsString(unit_item)) {
        cJSON_Delete(root);
        return -1;
    }
    size_t unit_len = strlen(unit_item->valuestring);

    // unit_len = MIN(unit_len, sizeof(ti->unit) - 1);
    strncpy(ti->unit, unit_item->valuestring, 1);
    ti->unit[unit_len] = '\0';
    cJSON_Delete(root);

    return 0;
}

static void MqttConnEventHandleTask(void *arg)
{
    (void)arg;
    g_uploadTimeInterval = DEFAULT_UPLOAD_TIME_INTERVAL;
    EventGroupHandle_t mqtt_conn_event_handle = MqttGetConnEventHandle();
    QueueHandle_t transceiver_queue_handle = TransceiverGetSensorQueueHandle();
    SensorDataFrame_t frame;
    while(true) {
        EventBits_t bits = xEventGroupWaitBits(mqtt_conn_event_handle, 
                                               MQTT_CONNECTED_BIT|MQTT_DISCONNECTED_BIT, 
                                               pdTRUE, 
                                               pdFALSE, 
                                               portMAX_DELAY);
        if ((bits & MQTT_CONNECTED_BIT) == MQTT_CONNECTED_BIT) {
            /* connect mqtt broker success */
            ESP_LOGI(MQTT_LORA, "Connect MQTT Broker.");
            MqttClientSubscribeSingle(MQTT_SUBSCRIBE_TOPIC, 0);
            while (true)
            {
                if (xQueueReceive(transceiver_queue_handle, &frame, portMAX_DELAY) == pdTRUE) {
                    char *pyload = MqttSoilSensorPublishPayload(&frame);
                    MqttClientPublish(MQTT_PUBLLISH_TOPPIC, pyload, 1);
                    free(pyload);
                }

                if ((bits & MQTT_DISCONNECTED_BIT) == MQTT_DISCONNECTED_BIT) {
                    /* disconnect mqtt broker success */
                    ESP_LOGI(MQTT_LORA, "Disconnect MQTT Broker.");
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(g_uploadTimeInterval));
            }
            /* 处理断线重连 */

            /* 处理断线重连 */
        }
    }
}

static void MqttSubQueueHandleTask(void *arg)
{
    (void)arg;
    MqttSubData_t sub_data;
    TimeIntervalSetting_t time_setting;
    QueueHandle_t queue_handle = MqttGetSubQueueHandle();
    while (true)
    {
        if (xQueueReceive(queue_handle, &sub_data, portMAX_DELAY) == pdTRUE) {
            MqttAnalysisSubPyload(sub_data.data, &time_setting);
            switch (time_setting.unit[0]) {
                case 'S':
                    g_uploadTimeInterval = time_setting.time * S;
                    break;
                case 'M':
                    g_uploadTimeInterval = time_setting.time * M;
                    break;
                case 'H':
                    g_uploadTimeInterval = time_setting.time * H;
                    break;
                default:
                    g_uploadTimeInterval = DEFAULT_UPLOAD_TIME_INTERVAL;
                    break;
            }
        }
    }
    
}

void MqttLoraInit(void)
{
    MqttInit(MQTT_URI, MQTT_USERNAME, MQTT_PASSWD, MQTT_CLIENT_ID);
}

void NewMqttConnEventHandleTask(void)
{
    xTaskCreate(MqttConnEventHandleTask, "mqtt_conn_event", 1024 * 5, NULL, 24, NULL);
}

void NewMqttSubQueueHandleTask(void)
{
    xTaskCreate(MqttSubQueueHandleTask, "mqtt_sub_handle", 1024 * 5, NULL, 24, NULL);
}