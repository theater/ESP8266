#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__
#include "mqtt.h"
#include "D:\HomeAutomation\ESP8266\security.h"
// security.h has definition of SSID and Password for Wi-fi

/*
 * GPIO2 => DHT22
 * GPIO4 => DS18B20
 * GPIO5 => NOT IN USE
 * GPIO12 => FAN
 * GPIO13 => HEATER
 * GPIO14 => BOILER
 */



// HTTPD USER CONFIG
#define USE_WIFI_MODE		STATIONAP_MODE
#define WIFI_CLIENTSSID		WIFI_SSID
#define WIFI_CLIENTPASSWORD	WIFI_PASSWORD
#define WIFI_AP_NAME		"ESP8266"
#define WIFI_AP_PASSWORD	"00000000"
#define PLATFORM_DEBUG		true

#define CFG_HOLDER	0x00FF55A4  /* Change this value to load default configurations */
#define CFG_LOCATION	0x3C	/* Please don't change or if you know what you doing */

// MQTT USER CONFIG

#define MQTT_HOST			"192.168.254.30" //or "192.168.11.1"
#define MQTT_PORT			1883
#define MQTT_BUF_SIZE		1024
#define MQTT_KEEPALIVE		60	 /*second*/

#define MQTT_CLIENT_ID		"BR_ESP_TEST"
#define MQTT_USER			""
#define MQTT_PASS			""


#define STA_SSID WIFI_CLIENTSSID
#define STA_PASS WIFI_CLIENTPASSWORD
#define STA_TYPE AUTH_WPA2_PSK

#define MQTT_RECONNECT_TIMEOUT 	5	/*second*/

#define DEFAULT_SECURITY	0

#define QUEUE_BUFFER_SIZE		 		2048

// Static definitions from MQTT user_init.c
#define DS18B20_MQTT_Temperature "BR_BOILER_TEMP_SENSOR"
#define DHT22_MQTT_Temperature "BR_TEMP_SENSOR"
#define DHT22_MQTT_Humidity "BR_H_SENSOR"


// GPIO settings - all GPIOs set as GPIO
#define PIN_GPIO0 0
#define PIN_GPIO0_MUX PERIPHS_IO_MUX_GPIO0_U
#define PIN_GPIO0_FUNC FUNC_GPIO0
#define PIN_GPIO0_TOPIC ""
#define PIN_GPIO0_TOPIC_CB ""


#define PIN_GPIO2 2
#define PIN_GPIO2_MUX PERIPHS_IO_MUX_GPIO2_U
#define PIN_GPIO2_FUNC FUNC_GPIO2
#define PIN_GPIO2_TOPIC ""
#define PIN_GPIO2_TOPIC_CB ""


#define PIN_GPIO4 4
#define PIN_GPIO4_MUX PERIPHS_IO_MUX_GPIO4_U
#define PIN_GPIO4_FUNC FUNC_GPIO4
#define PIN_GPIO4_TOPIC ""
#define PIN_GPIO4_TOPIC_CB ""

#define PIN_GPIO5 5
#define PIN_GPIO5_MUX PERIPHS_IO_MUX_GPIO5_U
#define PIN_GPIO5_FUNC FUNC_GPIO5
#define PIN_GPIO5_TOPIC "BR_FAN"
#define PIN_GPIO5_TOPIC_CB "BR_FAN_CB"

#define PIN_GPIO12 12
#define PIN_GPIO12_MUX PERIPHS_IO_MUX_MTDI_U
#define PIN_GPIO12_FUNC FUNC_GPIO12
#define PIN_GPIO12_TOPIC "BR_HEATER"
#define PIN_GPIO12_TOPIC_CB "BR_HEATER_CB"

#define PIN_GPIO13 13
#define PIN_GPIO13_MUX PERIPHS_IO_MUX_MTCK_U
#define PIN_GPIO13_FUNC FUNC_GPIO13
#define PIN_GPIO13_TOPIC "BR_INPUT_PIN1"
#define PIN_GPIO13_TOPIC_CB ""

#define PIN_GPIO14 14
#define PIN_GPIO14_MUX PERIPHS_IO_MUX_MTMS_U
#define PIN_GPIO14_FUNC FUNC_GPIO14
#define PIN_GPIO14_TOPIC "BR_BOILER"
#define PIN_GPIO14_TOPIC_CB "BR_BOILER_CB"


#define BR_BOILER_SET "BR_BOILER_SET"
#define BR_TEMP_ROOM_SET "BR_TEMP_ROOM_SET"
#define BR_HUMIDITY_SET "BR_HUMIDITY_SET"
#define BR_MODE "BR_Mode"
#define BR_MODE_CB "BR_Mode_CB"

int dBR_BOILER_SET;
int dBR_TEMP_ROOM_SET;
int dBR_HUMIDITY_SET;
int dBR_MODE;
int dDEVICE_MODE; // This mode defines if device uses remote (MQTT) management or uses local logics - 0 for remote, 1 for local mode. DEFAULT is 0

MQTT_Client mqttClient;
char currGPIO2State;
char currGPIO4State;
char currGPIO5State;
char currGPIO12State;
char currGPIO13State;
char currGPIO14State;
char DS18B20_Temperature[10];
char DHT22_Temperature[8];
char DHT22_Humidity[8];

#define sleepms(x) os_delay_us(x*1000);


#endif
