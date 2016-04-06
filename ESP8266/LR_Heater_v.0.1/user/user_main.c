

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

// HTTPD includes
#include "espmissingincludes.h"
#include "ets_sys.h"
#include "osapi.h"
#include "httpd.h"
#include "io.h"
#include "httpdespfs.h"
#include "cgi.h"
#include "cgiwifi.h"
#include "stdout.h"
#include "auth.h"
#include "wifi.h"
// MQTT includes
#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "user/mqtt.h"
#include "wifi.h"
#include "user/config.h"
#include "user/debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "driver/ds18b20.h"
#include <stdlib.h>
#include "debug.h"
#include "user_config.h"
#include "ntp/ntp.h"

void user_rf_pre_init(void) { return; }

// DS18B20
#include <os_type.h>
#define DELAY 60000 // milliseconds
#define NTPDELAY 30000 // milliseconds
LOCAL os_timer_t ds18b20_timer;
LOCAL os_timer_t NTP_timer;
extern int ets_uart_printf(const char *fmt, ...);
int (*console_printf)(const char *fmt, ...) = ets_uart_printf;

int ds18b20();
LOCAL void ICACHE_FLASH_ATTR ds18b20_cb(void *arg)
{
	ds18b20();
}/**/
// Static definitions from HTTPD user_init.c
// none

//=============================================================
// 		HTTPD Function Definitions
//=============================================================

//Function that tells the authentication system what users/passwords live on the system.
//This is disabled in the default build; if you want to try it, enable the authBasic line in
//the builtInUrls below.
int myPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass, int passLen) {
	if (no==0) {
		os_strcpy(user, "admin");
		os_strcpy(pass, "s3cr3t");
		return 1;
//Add more users this way. Check against incrementing no for each user added.
//	} else if (no==1) {
//		os_strcpy(user, "user1");
//		os_strcpy(pass, "something");
//		return 1;
	}
	return 0;
}


/*
This is the main url->function dispatching data struct.
In short, it's a struct with various URLs plus their handlers. The handlers can
be 'standard' CGI functions you wrote, or 'special' CGIs requiring an argument.
They can also be auth-functions. An asterisk will match any url starting with
everything before the asterisks; "*" matches everything. The list will be
handled top-down, so make sure to put more specific rules above the more
general ones. Authorization things (like authBasic) act as a 'barrier' and
should be placed above the URLs they protect.
*/
HttpdBuiltInUrl builtInUrls[]={
	{"/", cgiRedirect, "/index.tpl"},
	{"/gpio2.tpl", cgiEspFsTemplate, tplGPIO},
	{"/gpio4.tpl", cgiEspFsTemplate, tplGPIO},
	{"/index.tpl", cgiEspFsTemplate, tplIndex},
	{"/gpio2.cgi", cgiGPIO2, NULL},
	{"/gpio4.cgi", cgiGPIO4, NULL},

	//Routines to make the /wifi URL and everything beneath it work.

//Enable the line below to protect the WiFi configuration with an username/password combo.
//	{"/wifi/*", authBasic, myPassFn},

	{"/wifi", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/wifiscan.cgi", cgiWiFiScan, NULL},
	{"/wifi/wifi.tpl", cgiEspFsTemplate, tplWlan},
	{"/wifi/connect.cgi", cgiWiFiConnect, NULL},
	{"/wifi/setmode.cgi", cgiWifiSetMode, NULL},

	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};

//=============================================================
// 		MQTT Function Definitions
//=============================================================

void ICACHE_FLASH_ATTR wifiConnectCb(uint8_t status)
{
	if(status == STATION_GOT_IP){
		MQTT_Connect(&mqttClient);
	}
}

void ICACHE_FLASH_ATTR mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Connected\r\n");
	MQTT_Subscribe(&mqttClient, PIN_GPIO4_TOPIC, 0);
}

void ICACHE_FLASH_ATTR mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Disconnected\r\n");
}

void ICACHE_FLASH_ATTR mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Published\r\n");
}

// same as callback function in Arduino
void ICACHE_FLASH_ATTR mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	char topicBuf[64], dataBuf[64];
	MQTT_Client* client = (MQTT_Client*)args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	// HERE STARTS THE BASIC LOGIC BY KONSTANTIN
// GPIO2 handling here
/*	if (strcmp(topicBuf,PIN_GPIO2_TOPIC)==0) {
		if (strcmp(dataBuf,"OFF")==0) {
		GPIO_OUTPUT_SET(PIN_GPIO2, 0);
		INFO("GPIO2 set to OFF\r\n");
		MQTT_Publish(&mqttClient,PIN_GPIO2_TOPIC_CB,"OFF",3,0,0);
		currGPIO2State=0;
	} else if (strcmp(dataBuf,"ON")==0) {
		GPIO_OUTPUT_SET(PIN_GPIO2, 1);
		INFO("GPIO2 set to ON\r\n");
		MQTT_Publish(&mqttClient,PIN_GPIO2_TOPIC_CB,"ON",2,0,0);
		currGPIO2State=1;
		}
	}  /*  */
// GPIO4 handling here
	if (strcmp(topicBuf,PIN_GPIO4_TOPIC)==0) {
		if (strcmp(dataBuf,"OFF")==0) {
		GPIO_OUTPUT_SET(PIN_GPIO4, 0);
		INFO("GPIO4 set to OFF\r\n");
		MQTT_Publish(&mqttClient,PIN_GPIO4_TOPIC_CB,"OFF",3,0,0);
		currGPIO4State=0;
	} else if (strcmp(dataBuf,"ON")==0) {
		GPIO_OUTPUT_SET(PIN_GPIO4, 1);
		INFO("GPIO4 set to ON\r\n");
		MQTT_Publish(&mqttClient,PIN_GPIO4_TOPIC_CB,"ON",2,0,0);
		currGPIO4State=1;
		}
	}


	// HERE ENDS THE BASIC LOGIC BY KONSTANTIN
//	INFO("GPIO2 State: %d",GPIO_INPUT_GET(PIN_GPIO));
	INFO("MQTT topic: %s, data: %s \r\n", topicBuf, dataBuf);
//	os_free(topicBuf);
//	os_free(dataBuf);
}
//===========================================================
//		Kosio implementations
//===========================================================
int ICACHE_FLASH_ATTR ds18b20()
{
	int r, i;
	uint8_t addr[8], data[12];
	ds_init();

	r = ds_search(addr);
	if(r)
	{
		console_printf("Found Device @ %02x %02x %02x %02x %02x %02x %02x %02x\r\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
		if(crc8(addr, 7) != addr[7])
			console_printf( "CRC mismatch, crc=%xd, addr[7]=%xd\r\n", crc8(addr, 7), addr[7]);

		switch(addr[0])
		{
		case 0x10:
			console_printf("Device is DS18S20 family.\r\n");
			break;

		case 0x28:
			console_printf("Device is DS18B20 family.\r\n");
			break;

		default:
			console_printf("Device is unknown family.\r\n");
			return 1;
		}
	}
	else {
		console_printf("No DS18B20 detected, sorry.\r\n");
		return 1;
	}
	// perform the conversion
	reset();
	select(addr);

	write(DS1820_CONVERT_T, 1); // perform temperature conversion

	sleepms(1000); // sleep 1s

	console_printf("Scratchpad: ");
	reset();
	select(addr);
	write(DS1820_READ_SCRATCHPAD, 0); // read scratchpad

	for(i = 0; i < 9; i++)
	{
		data[i] = read();
		console_printf("%2x ", data[i]);
	}
	console_printf("\r\n");

	int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;
	LowByte = data[0];
	HighByte = data[1];
	TReading = (HighByte << 8) + LowByte;
	SignBit = TReading & 0x8000;  // test most sig bit
	if (SignBit) // negative
		TReading = (TReading ^ 0xffff) + 1; // 2's comp

	Whole = TReading >> 4;  // separate off the whole and fractional portions
	Fract = (TReading & 0xf) * 100 / 16;

	console_printf("Temperature: %c%d.%d Celsius\r\n", SignBit ? '-' : '+', Whole, Fract < 10 ? 0 : Fract);
	if(SignBit){
		os_sprintf(sTemperature, "%c%d.%d", SignBit ? '-' : '+', Whole, Fract < 10 ? 0 : Fract );
		MQTT_Publish(&mqttClient,LRHEATER_MQTT_Temperature,sTemperature,6,0,0);
	} else {
		os_sprintf(sTemperature, "%d.%d", Whole, Fract < 10 ? 0 : Fract );
		MQTT_Publish(&mqttClient,LRHEATER_MQTT_Temperature,sTemperature,5,0,0);
	}
	return r;
}

/**/


//Main routine. Initialize stdout, the I/O and the webserver and we're done.
void user_init(void) {
// HTTPD
	stdoutInit();
	ioInit();
	httpdInit(builtInUrls, 80);
//MQTT
	uart_init(115200, 115200);
	CFG_Load();
	sleepms(1000);

	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, SEC_NONSSL);
	//MQTT_InitConnection(&mqttClient, "192.168.11.122", 1880, 0);
	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
	//MQTT_InitClient(&mqttClient, "client_id", "user", "pass", 120, 1);

//	MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);

	INFO("device_ID:%s\r\n",sysCfg.device_id);
	INFO("MQTTHOST:%s\r\n",sysCfg.mqtt_host);
//DS18B20 timers
	os_timer_disarm(&ds18b20_timer);
	os_timer_setfn(&ds18b20_timer, (os_timer_func_t *)ds18b20_cb, (void *)0);
	os_timer_arm(&ds18b20_timer, DELAY, 1);

//NTP timers
		ntp_get_time();
		os_timer_disarm(&NTP_timer);
		os_timer_setfn(&NTP_timer, (os_timer_func_t *)ntp_get_time, (void *)0);
		os_timer_arm(&NTP_timer, NTPDELAY, 1);

// initialize GPIO2
	PIN_FUNC_SELECT(PIN_GPIO2_MUX, PIN_GPIO2_FUNC);
	GPIO_OUTPUT_SET(PIN_GPIO2, 0);
	INFO("GPIO2 set to OFF\r\n");

	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);

	os_printf("\nReady\n");
}
