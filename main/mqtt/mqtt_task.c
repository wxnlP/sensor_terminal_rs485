#include "mqtt_task.h"

#include "mqtt_client.h"
#include "esp_log.h"


static EventGroupHandle_t g_mqttConnEventHandle;
static QueueHandle_t g_mqttSubQueueHandle;
static esp_mqtt_client_handle_t g_mqttClientHandle;
static const char *MQTT_TASK = "mqtt_task";

static void mqtt_event_handle(void* event_handler_arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    // esp_mqtt_client_handle_t client = event->client;
    MqttSubData_t sub_data;
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(MQTT_TASK, "Trigger event: MQTT_EVENT_CONNECTED");
            /* 成功连接 MQTT broker */
            xEventGroupSetBits(g_mqttConnEventHandle, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(MQTT_TASK, "Trigger event: MQTT_EVENT_DISCONNECTED");
            /* MQTT broker断开连接 */
            xEventGroupSetBits(g_mqttConnEventHandle, MQTT_DISCONNECTED_BIT);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(MQTT_TASK, "Trigger event: MQTT_EVENT_SUBSCRIBED");
            /* 话题订阅成功 */
            ESP_LOGI(MQTT_TASK, "Subscribe topic sucess.");
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(MQTT_TASK, "Trigger event: MQTT_EVENT_UNSUBSCRIBED");
            /* 取消话题订阅成功 */
            ESP_LOGI(MQTT_TASK, "Unsubscribe topic sucess.");
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(MQTT_TASK, "Trigger event: MQTT_EVENT_PUBLISHED");
            /* 发布消息成功 */
            ESP_LOGI(MQTT_TASK, "Publish message sucess.");
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(MQTT_TASK, "Trigger event: MQTT_EVENT_DATA");
            /* 收到订阅话题的消息 */
            strncpy(sub_data.topic, event->topic, event->topic_len);
            sub_data.topic[event->topic_len] = '\0'; 
            sub_data.topic_len = event->topic_len; 

            memcpy(sub_data.data, event->data, event->data_len);
            sub_data.data_len = event->data_len;

            xQueueSend(g_mqttSubQueueHandle, &sub_data, 0);
            break;
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(MQTT_TASK, "Trigger event: MQTT_EVENT_BEFORE_CONNECT");
            /* 用于在连接之前修改客户端配置（较少用） */
            break;
        case MQTT_EVENT_DELETED:
            ESP_LOGI(MQTT_TASK, "Trigger event: MQTT_EVENT_DELETED");
            break;
        case MQTT_USER_EVENT:
            ESP_LOGI(MQTT_TASK, "Trigger event: MQTT_USER_EVENT");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(MQTT_TASK, "Trigger event: MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(MQTT_TASK, "Event ID: %d", event->event_id);
            break;
    }
}

void MqttInit(const char* uri, const char* user_name, const char* passwd, const char* client_id)
{
    esp_mqtt_client_config_t config = {
        .broker.address.uri = uri,
        .credentials.username = user_name,
        .credentials.client_id = client_id,
        .credentials.authentication.password = passwd,
        .session.disable_clean_session = false,
        .session.protocol_ver = MQTT_PROTOCOL_V_3_1_1,
        .session.keepalive = 60,
        // .session.last_will
        .network.reconnect_timeout_ms = 5000,
        .buffer.out_size = 1024 * 4,
        // .network.timeout_ms = 10000,
    };

    g_mqttClientHandle = esp_mqtt_client_init(&config);
    esp_mqtt_client_register_event(g_mqttClientHandle, MQTT_EVENT_ANY, mqtt_event_handle, NULL);
    ESP_ERROR_CHECK(esp_mqtt_client_start(g_mqttClientHandle));

    g_mqttConnEventHandle = xEventGroupCreate();
    g_mqttSubQueueHandle = xQueueCreate(10, sizeof(MqttSubData_t));
}

EventGroupHandle_t MqttGetConnEventHandle(void)
{
    return g_mqttConnEventHandle;
}

QueueHandle_t MqttGetSubQueueHandle(void)
{
    return g_mqttSubQueueHandle;
}

int MqttClientPublish(const char* topic, const char *data, int qos)
{
    return esp_mqtt_client_publish(g_mqttClientHandle, topic, data, 0, qos, 0);
}

int MqttClientSubscribeSingle(const char* topic, int qos)
{
    return esp_mqtt_client_subscribe(g_mqttClientHandle, topic, qos);
}