#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"

#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"
#include "wifihandler.h"

#include "timekeeper.h"

#define TAG "TIMEKEEPER"

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

#define SNTP_TIME_SERVER "pool.ntp.org"

systime_t _systime;
bool _timeInitialized = false;
SemaphoreHandle_t timekeeperDataMutex;

time_t now;

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(SNTP_TIME_SERVER);
    config.start = false;                       // start SNTP service explicitly (after connecting)
    config.server_from_dhcp = true;             // accept NTP offers from DHCP server, if any (need to enable *before* connecting)
    config.renew_servers_after_new_IP = true;   // let esp-netif update configured SNTP server(s) after receiving DHCP lease
    config.index_of_first_server = 1;           // updates from server num 1, leaving server 0 (from DHCP) intact
    // configure the event on which we renew servers
    esp_netif_sntp_init(&config);
    ESP_LOGI(TAG, "Starting SNTP");
    esp_netif_sntp_start();
}

static void timekeeper_task(void *pvParameters) {
    int retry = 0;
    const int retry_count = 10;
    _systime.date.year = 1900;
    _systime.date.month = 1;
    _systime.date.day = 1;
    _systime.time.hour = 00;
    _systime.time.min = 00;
    _systime.time.sec = 00;

    struct tm timeinfo;
    setenv("TZ", "CET-1CEST,M3.2.0/2,M11.1.0", 1);
    tzset();
    time(&now);    
    localtime_r(&now, &timeinfo);
    ESP_LOGI(TAG, "Initialize SNTP");
    initialize_sntp();
    xSemaphoreGive(timekeeperDataMutex);
    while(true) {
        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
        xSemaphoreTake(timekeeperDataMutex, portMAX_DELAY);
        if(_timeInitialized == false) {            
            while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
                ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
            }
            time(&now);
            localtime_r(&now, &timeinfo);
            ESP_LOGI(TAG, "Got Time from Timeserver. %02i:%02i:%02i", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            _timeInitialized = true;
        }
        xSemaphoreGive(timekeeperDataMutex);
        vTaskDelay(1000/portTICK_PERIOD_MS);
        if((xEventGroupGetBits(wifi_event_group) & WIFI_CONNECTED_BIT) != WIFI_CONNECTED_BIT) {
            ESP_LOGI(TAG, "WIFI Lost, reset _timeInitialized");
            _timeInitialized = false;
        }
    }
}

void init_timekeeper(void) {
    timekeeperDataMutex = xSemaphoreCreateMutex();
    xTaskCreate(&timekeeper_task, "timekeeper_task", 8192, NULL, 4, NULL);
}
void timekeeper_SetTime(systime_t *time) {
    xSemaphoreTake(timekeeperDataMutex, portMAX_DELAY);
    _systime.date.year = time->date.year;
    _systime.date.month = time->date.month;
    _systime.date.day = time->date.day;
    _systime.time.hour = time->time.hour;
    _systime.time.min = time->time.min;
    _systime.time.sec = time->time.sec;
    _timeInitialized = true;
    xSemaphoreGive(timekeeperDataMutex);
}
void timekeeper_GetTime(systime_t *returntime) {
    xSemaphoreTake(timekeeperDataMutex, portMAX_DELAY);
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);    
    returntime->date.year = timeinfo.tm_year + 1900;
    returntime->date.month = timeinfo.tm_mon + 1;
    returntime->date.day = timeinfo.tm_mday;
    returntime->time.hour = timeinfo.tm_hour;
    returntime->time.min = timeinfo.tm_min;
    returntime->time.sec = timeinfo.tm_sec;
    //ESP_LOGI(TAG, "timekeeperGetTime: %i:%i:%i - %i:%i:%i", returntime->date.year, returntime->date.month, returntime->date.day, returntime->time.hour, returntime->time.min, returntime->time.sec);
    xSemaphoreGive(timekeeperDataMutex);
}

int timekeeper_GetUTCDeviation() {
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    if(timeinfo.tm_isdst == 1) {
        return 2;
    } else {
        return 1;
    }
}

bool timekeeper_TimeInitialized(void) {
    return _timeInitialized;
}