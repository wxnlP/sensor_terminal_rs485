#include "wifi_station.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
// #include "esp_wifi_internal.h"


/* ------------------------------------ 宏定义 ------------------------------------- */
// #define WIFI_SSID                   "Mate70"
// #define WIFI_PASSWORD               "123456789"
#define WIFI_SSID                   "ChinaNet-5854"
#define WIFI_PASSWORD               "ct123456"
#define WIFI_CONNECT_MAX_TIMES      10
#define WIFI_CONNECTED_EVENT_BIT    (1<<0)
#define WIFI_DISCONNECTED_EVENT_BIT (1<<1)

/* ------------------------------------ 全局变量 ------------------------------------ */
static const char *WIFI_TAG = "wifi_sta";
static EventGroupHandle_t g_wifiEventGroupHandle;



static void wifi_evnet_handler(void* event_handler_arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    ESP_LOGI(WIFI_TAG, "Trigger wifi event handler");

    /* 判断触发的wifi事件 */
    static uint8_t try_times = 0;
    // 完成启动阶段，WIFI_EVENT_STA_START事件被加到默认事件循环
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // 连接 wifi
        ESP_ERROR_CHECK(esp_wifi_connect());
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupSetBits(g_wifiEventGroupHandle, WIFI_DISCONNECTED_EVENT_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ip_event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(WIFI_TAG, "Get IP:" IPSTR, IP2STR(&ip_event->ip_info.ip));
        try_times = 0;
        xEventGroupSetBits(g_wifiEventGroupHandle, WIFI_CONNECTED_EVENT_BIT);
    }

}


void WifiStaInit(void)
{
    /* -------------------- WIFI 初始化配置 -------------------- */

    ESP_LOGI(WIFI_TAG, "1.初始化阶段");
    // 初始化 LwIP
    esp_netif_init();
    // 创建默认事件循环
    esp_event_loop_create_default();
    // 创建默认 WiFi sta 网络接口
    esp_netif_create_default_wifi_sta();
    // 初始化 WiFi 驱动
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&config);

    // 注册 WIFI_EVENT 事件组中的所有事件到默认事件循环
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                         wifi_evnet_handler, NULL, &instance_any_id);

    // 注册 IP_EVENT 事件组中的 IP_EVENT_STA_GOT_IP 事件到默认事件循环
    esp_event_handler_instance_t instance_get_ip;
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                                        wifi_evnet_handler, NULL, &instance_get_ip);

    
    
    ESP_LOGI(WIFI_TAG, "2.配置阶段");
    // 设置 WiFi 工作模式
    esp_wifi_set_mode(WIFI_MODE_STA);
    // WiFi 初始化
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            // auth mode（认证模式）默认为 WPA2，即password >= 8
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .threshold.rssi = -85,
        }
    };
    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_set_max_tx_power(84);
    // esp_wifi_internal_set_rate_low_power


    ESP_LOGI(WIFI_TAG, "3.启动阶段");
    // 启动 wifi
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // 完成初始化配置 
    ESP_LOGI(WIFI_TAG, "Wifi station mode init finish.");
    
    /* -------------------- 事件组标志位 -------------------- */
    // 创建一个事件组
    g_wifiEventGroupHandle = xEventGroupCreate();

    // 等待事件组事件
    EventBits_t uxBits = xEventGroupWaitBits(
                        g_wifiEventGroupHandle,
                        WIFI_CONNECTED_EVENT_BIT | WIFI_DISCONNECTED_EVENT_BIT,
                        pdFALSE,
                        pdFALSE,
                        portMAX_DELAY);
    
    // 判断结果
    if (uxBits & WIFI_CONNECTED_EVENT_BIT) {
        ESP_LOGI(WIFI_TAG, "WiFi %s 连接成功.", WIFI_SSID);
    } else if (uxBits & WIFI_DISCONNECTED_EVENT_BIT) {
        ESP_LOGE(WIFI_TAG, "WiFi %s 连接失败.", WIFI_SSID);
        esp_wifi_connect();
    } else {
        ESP_LOGW(WIFI_TAG, "Unexpected event.");
    }

}

void WifiStaReconnect(void)
{
    EventBits_t uxBits = xEventGroupWaitBits(
                            g_wifiEventGroupHandle,
                            WIFI_CONNECTED_EVENT_BIT | WIFI_DISCONNECTED_EVENT_BIT,
                            pdTRUE,
                            pdFALSE,
                            portMAX_DELAY);
    if (uxBits & WIFI_DISCONNECTED_EVENT_BIT) {
        while (true) {
            esp_wifi_connect();
            uxBits = xEventGroupWaitBits(
                        g_wifiEventGroupHandle,
                        WIFI_CONNECTED_EVENT_BIT | WIFI_DISCONNECTED_EVENT_BIT,
                        pdTRUE,
                        pdFALSE,
                        pdMS_TO_TICKS(500));
            if (uxBits & WIFI_CONNECTED_EVENT_BIT) {
                ESP_LOGI(WIFI_TAG, "WiFi %s 重新连接成功.", WIFI_SSID);
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

}
