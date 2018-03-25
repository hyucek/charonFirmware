#ifndef _MQTT_CLIENT_H_
#define _MQTT_CLIENT_H_

#include "espressif/esp_common.h"
#include "esp/uart.h"

#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <ssid_config.h>

#include <espressif/esp_sta.h>
#include <espressif/esp_wifi.h>

#include <paho_mqtt_c/MQTTESP8266.h>
#include <paho_mqtt_c/MQTTClient.h>

#include <semphr.h>


#define PUB_MSG_LEN 16

extern QueueHandle_t publish_queue;

extern void  beat_task(void *pvParameters);
extern void  mqtt_task(void *pvParameters);

#endif //_MQTT_CLIENT_H_