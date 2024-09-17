#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifihandler.h"

#define TAG "WifiHandler"

EventGroupHandle_t wifi_event_group;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    static uint32_t connection_retries = 0;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "Wifi Connected successfully, waiting for IP");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {   
        if(connection_retries == 0) {
            ESP_LOGW(TAG, "Lost Connection to WIFI");
        }
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        vTaskDelay(1000);
        if(connection_retries == 0) {
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        esp_wifi_connect();
        connection_retries++;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        connection_retries = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifihandler_task(void* param) {
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_HUNT_AND_PECK,
            .sae_h2e_identifier = "",
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "Waiting for wifi connect");
    for(;;) {
        vTaskDelay(1000);
    }
}

void init_wifihandler(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    xTaskCreate(&wifihandler_task, "wifihandler_task", 8000, NULL, 2, NULL);
}