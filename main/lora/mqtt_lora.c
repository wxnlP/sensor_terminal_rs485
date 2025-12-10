#include "mqtt_lora.h"
#include "cJSON.h"
#include "esp_log.h"
#include "lora/transceiver.h"
#include "lora/device_num.h"
#include "mqtt/mqtt_task.h"
#include <string.h>

#ifdef HUAWEI_CLOUD
#define MQTT_URI        "mqtt://9dfa5c258d.st1.iotda-device.cn-east-3.myhuaweicloud.com:1883"
#define MQTT_USERNAME   "677388cfbab900244b135588_DATAS"
#define MQTT_PASSWD     "ad31747e0b97a6ca1287043ca46a3571796f0d20e1056ecabec4f05fc2db545f"
#define MQTT_CLIENT_ID  "677388cfbab900244b135588_DATAS_0_0_2024123106"
#define MQTT_DEVICE_ID  "677388cfbab900244b135588_DATAS"
#define MQTT_SERVICE_ID_SOIL    "soil_sensor"
#define MQTT_SERVICE_ID_321     "321_sensor"
// HUAWEICLOUDE平台的话题定义
#define  MQTT_PUBLLISH_TOPIC_SOIL      "$oc/devices/677388cfbab900244b135588_DATAS/sys/properties/report"
#define  MQTT_PUBLLISH_TOPIC_321      "$oc/devices/677388cfbab900244b135588_DATAS/sys/properties/report"
#define  MQTT_RequestTopic      "$oc/devices/677388cfbab900244b135588_DATAS/sys/shadow/get/request_id={request_id}"
#define  MQTT_SUBSCRIBE_TOPIC    "$oc/devices/677388cfbab900244b135588_DATAS/sys/shadow/get/response/#"
#else
#define MQTT_URI                "mqtt://u2f8813a.ala.dedicated.huawei.emqxcloud.cn:1883"
// #define MQTT_URI                "mqtt://192.168.43.4:1883"
#define MQTT_USERNAME           "soilsensor"
#define MQTT_PASSWD             "admin123"
#define MQTT_CLIENT_ID          "frametest"
#define MQTT_SERVICE_ID_SOIL    "soil_sensor"
#define MQTT_SERVICE_ID_321     "321_sensor"
#define MQTT_PUBLLISH_TOPIC_SOIL   "soil/sensor"
#define MQTT_PUBLLISH_TOPIC_321    "321/sensor"
#define MQTT_SUBSCRIBE_TOPIC    "soil/time"
#endif

#define S (1000)
#define M (60 * S)
#define H (60 * M)
#define DEFAULT_UPLOAD_TIME_INTERVAL (10 * S)

static int g_uploadTimeInterval;
static const char *MQTT_LORA = "mqtt_lora";


static char *MqttSoilSensorPublishPayload(SensorDataFrame_t *frame)
{
    // 创建JSON结构
    cJSON *root = cJSON_CreateObject();
    cJSON *services = cJSON_CreateArray();

    // 构建服务节点
    cJSON *upload = cJSON_CreateObject();
    // 添加属性
    cJSON *props = cJSON_CreateObject();
    char index[16];
    if (frame->flag & (1 << 0)) {
        generate_ss_name(frame->index, index);
        cJSON_AddStringToObject(upload, "service_id", MQTT_SERVICE_ID_SOIL);
        cJSON_AddStringToObject(props, "index", index);
        cJSON_AddNumberToObject(props, "battery", frame->battery);
        cJSON_AddNumberToObject(props, "signal", frame->signal);
        cJSON_AddNumberToObject(props, "suc_signal", frame->suc_signal);
        cJSON_AddNumberToObject(props, "fail_signal", frame->fail_signal);
        cJSON_AddNumberToObject(props, "signal_count", frame->signal_count);
        cJSON_AddNumberToObject(props, "vaild_count", frame->vaild_count);
        cJSON_AddNumberToObject(props, "signal_count", frame->signal_count);
        cJSON_AddNumberToObject(props, "temperature", frame->t_soil);
        cJSON_AddNumberToObject(props, "humidity", frame->h_soil);
        cJSON_AddNumberToObject(props, "ec", frame->ec);
        cJSON_AddNumberToObject(props, "ph", frame->ph);
        cJSON_AddNumberToObject(props, "n", frame->n);
        cJSON_AddNumberToObject(props, "p", frame->p);
        cJSON_AddNumberToObject(props, "k", frame->k);
    } else if (frame->flag & (1 << 1)) {
        generate_pspc_name(frame->index, index);
        cJSON_AddStringToObject(upload, "service_id", MQTT_SERVICE_ID_321);
        cJSON_AddStringToObject(props, "index", index);
        cJSON_AddNumberToObject(props, "battery", frame->battery);
        cJSON_AddNumberToObject(props, "temperature", frame->t_321);
        cJSON_AddNumberToObject(props, "humidity", frame->h_321);
        cJSON_AddNumberToObject(props, "sunlight", frame->sunlight);
    }
    

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
    if (root == NULL) {
        // 如果解析失败，则立即返回，不进行任何 cJSON_Delete 或后续操作。
        // 这能安全地处理接收到的任何非 JSON 或格式错误的 payload。
        // ESP_LOGE(MQTT_TASK, "Payload is not valid JSON.");
        return -1;
    }
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
    // 假设 "unit" 字段仅包含一个字符
    const char *unit_string = unit_item->valuestring;
    
    // 确保字符串不为空
    if (unit_string == NULL || unit_string[0] == '\0') {
        cJSON_Delete(root);
        return -1; 
    }

    // 取字符串的第一个字符，赋值给结构体的 character 成员
    ti->unit = unit_string[0];

    cJSON_Delete(root);

    return 0;
}

static void MqttConnEventHandleTask(void *arg)
{
    (void)arg;
    g_uploadTimeInterval = DEFAULT_UPLOAD_TIME_INTERVAL;
    EventGroupHandle_t mqtt_conn_event_handle = MqttGetConnEventHandle();
    while (mqtt_conn_event_handle == NULL) {
        vTaskDelay(pdMS_TO_TICKS(100));
        mqtt_conn_event_handle = MqttGetConnEventHandle();
    }
    QueueHandle_t transceiver_queue_handle = TransceiverGetSensorQueueHandle();
    while (transceiver_queue_handle == NULL) {
        vTaskDelay(pdMS_TO_TICKS(100));
        transceiver_queue_handle = TransceiverGetSensorQueueHandle();
    }
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
                    if (frame.flag & (1 << 0)) {
                        MqttClientPublish(MQTT_PUBLLISH_TOPIC_SOIL, pyload, 1);
                    } else if (frame.flag & (1 << 1)) {
                        MqttClientPublish(MQTT_PUBLLISH_TOPIC_321, pyload, 1);
                    }
                    free(pyload);
                }

                if ((bits & MQTT_DISCONNECTED_BIT) == MQTT_DISCONNECTED_BIT) {
                    /* disconnect mqtt broker success */
                    ESP_LOGI(MQTT_LORA, "Disconnect MQTT Broker.");
                    break;
                }
                ESP_LOGI(MQTT_LORA, "Time interval: %llu", g_uploadTimeInterval);
            }
        }
    }
}

static void MqttSubQueueHandleTask(void *arg)
{
    (void)arg;
    MqttSubData_t sub_data;
    TimeIntervalSetting_t time_setting;
    QueueHandle_t queue_handle = MqttGetSubQueueHandle();
    while (queue_handle == NULL) {
        vTaskDelay(pdMS_TO_TICKS(100));
        queue_handle = MqttGetSubQueueHandle();
    }
    while (true)
    {
        if (xQueueReceive(queue_handle, &sub_data, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(MQTT_LORA, "Received topic:\n[%.*s]", sub_data.topic_len, sub_data.topic);
            ESP_LOGI(MQTT_LORA, "data:\n[%.*s]", sub_data.data_len, sub_data.data);
            
            int ret = MqttAnalysisSubPyload(sub_data.data, &time_setting);
            if (ret != -1) {
                printf("time setting: %d %d\n", time_setting.time, time_setting.unit);
                switch (time_setting.unit) {
                    case 'S':
                        ESP_LOGI(MQTT_LORA, "修改单位S");
                        g_uploadTimeInterval = time_setting.time * S;
                        break;
                    case 's':
                        ESP_LOGI(MQTT_LORA, "修改单位S");
                        g_uploadTimeInterval = time_setting.time * S;
                        break;
                    case 'M':
                        ESP_LOGI(MQTT_LORA, "修改单位M");
                        g_uploadTimeInterval = time_setting.time * M;
                        break;
                    case 'm':
                        ESP_LOGI(MQTT_LORA, "修改单位M");
                        g_uploadTimeInterval = time_setting.time * M;
                        break;
                    case 'H':
                        ESP_LOGI(MQTT_LORA, "修改单位H");
                        g_uploadTimeInterval = time_setting.time * H;
                        break;
                    case 'h':
                        ESP_LOGI(MQTT_LORA, "修改单位H");
                        g_uploadTimeInterval = time_setting.time * H;
                        break;
                    default:
                        ESP_LOGI(MQTT_LORA, "格式错误❌️");
                        g_uploadTimeInterval = DEFAULT_UPLOAD_TIME_INTERVAL;
                        break;
                }
            }
            
        }
    }
    
}

void MqttLoraInit(void)
{
    ESP_LOGI(MQTT_LORA, "%s", MQTT_URI);
    MqttInit(MQTT_URI, MQTT_USERNAME, MQTT_PASSWD, MQTT_CLIENT_ID);
}

int GetTimeInterval(void)
{
    return g_uploadTimeInterval;
}

void NewMqttConnEventHandleTask(void)
{
    xTaskCreate(MqttConnEventHandleTask, "mqtt_conn_event", 1024 * 5, NULL, 24, NULL);
}

void NewMqttSubQueueHandleTask(void)
{
    xTaskCreate(MqttSubQueueHandleTask, "mqtt_sub_handle", 1024 * 5, NULL, 24, NULL);
}