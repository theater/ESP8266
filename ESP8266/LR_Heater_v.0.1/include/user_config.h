
#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__
#include "mqtt.h"

// HTTPD USER CONFIG
#define USE_WIFI_MODE		STATIONAP_MODE
#define WIFI_CLIENTSSID		"KLP"
#define WIFI_CLIENTPASSWORD	"charly78"
#define WIFI_AP_NAME		"LR_HEATER1"
#define WIFI_AP_PASSWORD	"00000000"
#define PLATFORM_DEBUG		true

#define CFG_HOLDER	0x00FF55A4  /* Change this value to load default configurations */
#define CFG_LOCATION	0x3C	/* Please don't change or if you know what you doing */

// MQTT USER CONFIG

#define MQTT_HOST			"192.168.254.30" //or "192.168.11.1"
#define MQTT_PORT			1883
#define MQTT_BUF_SIZE		1024
#define MQTT_KEEPALIVE		60	 /*second*/

#define MQTT_CLIENT_ID		"LR_HEATER1"
#define MQTT_USER			""
#define MQTT_PASS			""


#define STA_SSID WIFI_CLIENTSSID
#define STA_PASS WIFI_CLIENTPASSWORD
#define STA_TYPE AUTH_WPA2_PSK

#define MQTT_RECONNECT_TIMEOUT 	5	/*second*/

#define DEFAULT_SECURITY	0

#define QUEUE_BUFFER_SIZE		 		2048

// Static definitions from MQTT user_init.c
// GPIO settings - all GPIOs set as GPIO
// DS18B20
#define PIN_GPIO2 2
#define PIN_GPIO2_MUX PERIPHS_IO_MUX_GPIO2_U
#define PIN_GPIO2_FUNC FUNC_GPIO2
//#define PIN_GPIO2_TOPIC "LR_HEATER1"
//#define PIN_GPIO2_TOPIC_CB "LR_HEATER1_CB"

// Output pin for the relay
#define PIN_GPIO4 4
#define PIN_GPIO4_MUX PERIPHS_IO_MUX_MTDI_U
#define PIN_GPIO4_FUNC FUNC_GPIO4
#define PIN_GPIO4_TOPIC "LR_HEATER1"
#define PIN_GPIO4_TOPIC_CB "LR_HEATER1_CB"

#define PIN_GPIO5 5
#define PIN_GPIO5_MUX PERIPHS_IO_MUX_GPIO5_U
#define PIN_GPIO5_FUNC FUNC_GPIO5

#define PIN_GPIO12 12
#define PIN_GPIO12_MUX PERIPHS_IO_MUX_MTDI_U
#define PIN_GPIO12_FUNC FUNC_GPIO12

#define PIN_GPIO13 13
#define PIN_GPIO13_MUX PERIPHS_IO_MUX_MTCK_U
#define PIN_GPIO13_FUNC FUNC_GPIO13

#define PIN_GPIO14 14
#define PIN_GPIO14_MUX PERIPHS_IO_MUX_MTDI_U
#define PIN_GPIO14_FUNC FUNC_GPIO14

#define LRHEATER_MQTT_Temperature "LRHEATER_MQTT_Temperature"

MQTT_Client mqttClient;
char currGPIO2State;
char currGPIO4State;
char currGPIO5State;
char currGPIO12State;
char currGPIO13State;
char currGPIO14State;
char sTemperature[10];
char sDate[20];

#define sleepms(x) os_delay_us(x*1000);


#endif
