#pragma once
#define WIFI_SSID "YOUR_WIFI"
#define WIFI_PASSWORD "YOUR_PASSWORD"
// LAN IP of backend computer, never localhost on ESP32.
#define API_BASE_URL "http://192.168.1.10:8000"
#define DEVICE_ID "bus-01"
#define DEVICE_API_KEY "CHANGE_TO_MATCH_BACKEND"
#define MQTT_HOST ""
#define MQTT_PORT 8883
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
// Paste the PEM root CA supplied by your broker. Never use setInsecure().
#define MQTT_ROOT_CA ""
