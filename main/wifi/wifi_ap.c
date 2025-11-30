#include "wifi_ap.h"

#include <string.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_mac.h"



/* ------------------------------------ 宏定义 ------------------------------------- */
#define WIFI_AP_SSID            "ESP32C6"
#define WIFI_AP_PASS            "123456789"
#define WIFI_AP_CHANNEL         11
#define WIFI_AP_MAX_CONN        10
#define WIFI_AP_GTK_INTERVAL    0

/* ------------------------------------ 全局变量 ------------------------------------ */
static const char* WIFI_TAG = "wifi_ap";


static void wifi_evnet_handler(void* event_handler_arg,
                               esp_event_base_t event_base, int32_t event_id,
                               void* event_data) 
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(WIFI_TAG, "station " MACSTR " join, AID=%d", 
                 MAC2STR(event->mac), event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(WIFI_TAG, "station " MACSTR " leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
} 

void WifiApInit(void) {
    /* -------------------- WIFI 初始化配置 -------------------- */

    ESP_LOGI(WIFI_TAG, "1.初始化阶段");
    // 初始化 LwIP
    esp_netif_init();
    // 创建默认事件循环
    esp_event_loop_create_default();
    // 创建默认 WiFi AP 网络接口
    esp_netif_create_default_wifi_ap();
    // 初始化 WiFi 驱动
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&config);
    // 注册 WIFI_EVENT 事件组中的所有事件到默认事件循环
    esp_event_handler_instance_register(
                                    WIFI_EVENT, 
                                    ESP_EVENT_ANY_ID,
                                    wifi_evnet_handler, 
                                    NULL, 
                                    NULL);

    ESP_LOGI(WIFI_TAG, "2.配置阶段");
    
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .ssid_len = strlen(WIFI_AP_SSID),
            .password = WIFI_AP_PASS,
            .channel = WIFI_AP_CHANNEL,
            .max_connection = WIFI_AP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg.required = true,
            .gtk_rekey_interval = WIFI_AP_GTK_INTERVAL,
        }
    };

    // 开放热点
    if (strlen(WIFI_AP_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    // 设置 WiFi 工作模式
    esp_wifi_set_mode(WIFI_MODE_AP);
    // WiFi 初始化
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    // 启动 WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    // 完成初始化配置 
    ESP_LOGI(WIFI_TAG, "Wifi AP mode init finish, ssid:%s password: %s", 
             WIFI_AP_SSID, WIFI_AP_PASS);

}