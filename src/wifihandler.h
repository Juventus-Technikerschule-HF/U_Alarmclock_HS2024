#pragma once

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
extern EventGroupHandle_t wifi_event_group;

#define WIFI_SSID "yourssid"
#define WIFI_PASSWORD "yourverysecurepassword"

void init_wifihandler(void);