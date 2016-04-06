

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

void user_rf_pre_init(void) { return; } // Added as mandatory after 1.x
LOCAL void handle_pin_press();
LOCAL void ICACHE_FLASH_ATTR wifiReconnectTimerCB();


// DS18B20
#include <os_type.h>
#define DELAY 60000 // milliseconds
LOCAL os_timer_t ds18b20_timer;

// DHT22
#include "driver/dht22.h"
LOCAL os_timer_t dht22_timer;
LOCAL os_timer_t wifiReconnectTimer;
extern int ets_uart_printf(const char *fmt, ...);
int (*console_printf)(const char *fmt, ...) = ets_uart_printf;
// INPUT PIN READ
LOCAL os_timer_t input_pin_timer;
LOCAL os_timer_t sntp_timer;
int prev_input_pin_state=0,curr_input_pin_state=0;
int button_counter=0;
int button_timeout=1300;
int button_debounce=125;
ip_addr_t NTP1;

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
	{"/heater.tpl", cgiEspFsTemplate, tplHEATER},
	{"/fan.tpl", cgiEspFsTemplate, tplFAN},
	{"/boiler.tpl", cgiEspFsTemplate, tplBOILER},
	{"/index.tpl", cgiEspFsTemplate, tplIndex},
	{"/HEATER.cgi", cgiHEATER, NULL},
	{"/FAN.cgi", cgiFAN, NULL},
	{"/BOILER.cgi", cgiBOILER, NULL},

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
	} else {
		INFO("No wifi available. Setting 1 min timer for retry...\n");
		os_timer_disarm(&wifiReconnectTimer);
		os_timer_setfn(&wifiReconnectTimer, (os_timer_func_t *)wifiReconnectTimerCB, (void *)0);
		os_timer_arm(&wifiReconnectTimer, DELAY, 1);
		INFO("Device MODE set to LOCAL !\r\n");
		dDEVICE_MODE=1;
	}
}

LOCAL void ICACHE_FLASH_ATTR wifiReconnectTimerCB() {
	os_timer_disarm(&wifiReconnectTimer);
	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);
}

void ICACHE_FLASH_ATTR mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Connected\r\n");
	MQTT_Subscribe(&mqttClient, PIN_GPIO5_TOPIC, 0);
	MQTT_Subscribe(&mqttClient, PIN_GPIO12_TOPIC, 0);
	MQTT_Subscribe(&mqttClient, PIN_GPIO14_TOPIC, 0);
	MQTT_Subscribe(&mqttClient, BR_BOILER_SET, 0);
	MQTT_Subscribe(&mqttClient, BR_TEMP_ROOM_SET, 0);
	MQTT_Subscribe(&mqttClient, BR_HUMIDITY_SET, 0);
	MQTT_Subscribe(&mqttClient, BR_MODE, 0);/**/
	dDEVICE_MODE=0;
	INFO("Device MODE set to AUTO !\r\n");
}

void ICACHE_FLASH_ATTR mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Disconnected\r\n");
	dDEVICE_MODE=1;
	INFO("Device MODE set to LOCAL !\r\n");
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
	if (strcmp(topicBuf,PIN_GPIO2_TOPIC)==0) {
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
	}
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
// GPIO12 handling here
	if (strcmp(topicBuf,PIN_GPIO12_TOPIC)==0) {
		if (strcmp(dataBuf,"OFF")==0) {
		GPIO_OUTPUT_SET(PIN_GPIO12, 0);
		INFO("GPIO12 set to OFF\r\n");
		MQTT_Publish(&mqttClient,PIN_GPIO12_TOPIC_CB,"OFF",3,0,0);
		currGPIO4State=0;
	} else if (strcmp(dataBuf,"ON")==0) {
		GPIO_OUTPUT_SET(PIN_GPIO12, 1);
		INFO("GPIO12 set to ON\r\n");
		MQTT_Publish(&mqttClient,PIN_GPIO12_TOPIC_CB,"ON",2,0,0);
		currGPIO12State=1;
		}
	}
// GPIO5 handling here
	if (strcmp(topicBuf,PIN_GPIO5_TOPIC)==0) {
		if (strcmp(dataBuf,"OFF")==0) {
		GPIO_OUTPUT_SET(PIN_GPIO5, 0);
		INFO("GPIO5 set to OFF\r\n");
		MQTT_Publish(&mqttClient,PIN_GPIO5_TOPIC_CB,"OFF",3,0,0);
		currGPIO4State=0;
	} else if (strcmp(dataBuf,"ON")==0) {
		GPIO_OUTPUT_SET(PIN_GPIO5, 1);
		INFO("GPIO5 set to ON\r\n");
		MQTT_Publish(&mqttClient,PIN_GPIO5_TOPIC_CB,"ON",2,0,0);
		currGPIO5State=1;
		}
	}
// GPIO14 handling here
	if (strcmp(topicBuf,PIN_GPIO14_TOPIC)==0) {
		if (strcmp(dataBuf,"OFF")==0) {
		GPIO_OUTPUT_SET(PIN_GPIO14, 0);
		INFO("GPIO14 set to OFF\r\n");
		MQTT_Publish(&mqttClient,PIN_GPIO14_TOPIC_CB,"OFF",3,0,0);
		currGPIO4State=0;
	} else if (strcmp(dataBuf,"ON")==0) {
		GPIO_OUTPUT_SET(PIN_GPIO14, 1);
		INFO("GPIO14 set to ON\r\n");
		MQTT_Publish(&mqttClient,PIN_GPIO14_TOPIC_CB,"ON",2,0,0);
		currGPIO14State=1;
		}
	}
//	BR_BOILER_SET
	if (strcmp(topicBuf,BR_BOILER_SET)==0) {
		dBR_BOILER_SET=atoi(dataBuf);
		INFO("BOILER SET TEMPERATURE RECEIVED: %d\r\n",dBR_BOILER_SET);
	}
	if (strcmp(topicBuf,BR_TEMP_ROOM_SET)==0) {
		dBR_TEMP_ROOM_SET=atoi(dataBuf);
		INFO("BATHROOM SET TEMPERATURE RECEIVED: %d\r\n",dBR_TEMP_ROOM_SET);
	}
	if (strcmp(topicBuf,BR_HUMIDITY_SET)==0) {
		dBR_HUMIDITY_SET=atoi(dataBuf);
		INFO("HUMIDITY SET RECEIVED: %d\r\n",dBR_HUMIDITY_SET);
	}
	if (strcmp(topicBuf,BR_MODE)==0) {
		dBR_MODE=atoi(dataBuf);
		INFO("BATHROOM MODE RECEIVED: %d\r\n",dBR_MODE);
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
	int r, i, tempLength=0;
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
	//MQTT stuff here
	console_printf("DS18B20/Temperature: %c%d.%d Celsius\r\n", SignBit ? '-' : '+', Whole, Fract < 10 ? 0 : Fract);
	if(SignBit){
		os_sprintf(DS18B20_Temperature, "%c%d.%d", SignBit ? '-' : '+', Whole, Fract < 10 ? 0 : Fract );
		MQTT_Publish(&mqttClient,DS18B20_MQTT_Temperature,DS18B20_Temperature,5,0,0);
	} else {
		os_sprintf(DS18B20_Temperature, "%d.%d", Whole, Fract < 10 ? 0 : Fract );
		MQTT_Publish(&mqttClient,DS18B20_MQTT_Temperature,DS18B20_Temperature,4,0,0);
	}
	// MANUAL MODE LOGICS HERE
	if (dDEVICE_MODE){
		if (Whole>=dBR_BOILER_SET&&GPIO_INPUT_GET(PIN_GPIO14)) {
			GPIO_OUTPUT_SET(PIN_GPIO14,0);
		}
	}
	return r;
}

/**/
LOCAL void ICACHE_FLASH_ATTR dht22_cb(void *arg)
{
	struct dht_sensor_data* r = DHTRead();
	int tempLength=0;
	int humLength=0;
	float lastTemp = r->temperature;
	float lastHum = r->humidity;
	if(r->success&&lastTemp>-30&&lastHum>0)
	{
		//TRANSFORMATION HERE
		console_printf("DHT22/Temperature: %d.%d *C, Humidity: %d.%d %%\r\n", (int)(lastTemp),(int)((lastTemp - (int)lastTemp)*100), (int)(lastHum),(int)((lastHum - (int)lastHum)*100));
		os_sprintf(DHT22_Temperature,"%d.%d", (int)(lastTemp),(int)((lastTemp - (int)lastTemp)*100));
		os_sprintf(DHT22_Humidity,"%d.%d", (int)(lastHum),(int)((lastHum - (int)lastHum)*100));
		if(lastTemp>=0) {
			if(lastTemp>9)
				tempLength=4;
			 else
				tempLength=3;
		} else tempLength=5; // assumption - bathroom temperature should not fall under -9 :P
		if(lastHum>9) humLength=2;
			else if (lastHum==100) humLength=3;
				else humLength=1;
		// MQTT PUBLISH HERE
		MQTT_Publish(&mqttClient,DHT22_MQTT_Temperature,DHT22_Temperature,tempLength,0,0);
		MQTT_Publish(&mqttClient,DHT22_MQTT_Humidity,DHT22_Humidity,humLength,0,0);
		// MANUAL MODE LOGICS HERE
		if (dDEVICE_MODE){
			// maintain currently set humidity
			if(lastHum<dBR_HUMIDITY_SET&&GPIO_INPUT_GET(PIN_GPIO5)&&!GPIO_INPUT_GET(PIN_GPIO13)) {
				GPIO_OUTPUT_SET(PIN_GPIO5, 0);
			} else if ((lastHum>dBR_HUMIDITY_SET+7)&&!GPIO_INPUT_GET(PIN_GPIO5)) {
				GPIO_OUTPUT_SET(PIN_GPIO5,1);
			}
			// maintain currently set bathroom temperature
			if(lastTemp>=dBR_TEMP_ROOM_SET&&GPIO_INPUT_GET(PIN_GPIO12)){
				GPIO_OUTPUT_SET(PIN_GPIO12,0);
			} else if ((lastTemp<dBR_TEMP_ROOM_SET-1)&&!GPIO_INPUT_GET(PIN_GPIO12)) {
				GPIO_OUTPUT_SET(PIN_GPIO12,1);
			}
		}
	}
	else
	{
		console_printf("Error reading temperature and humidity\r\n");
	}
}

LOCAL void ICACHE_FLASH_ATTR read_input_pin(void *arg)
{
	uint32 gpio_status;
	ETS_GPIO_INTR_DISABLE(); // Disable gpio interrupts
	sleepms(15);
	if(!GPIO_INPUT_GET(PIN_GPIO13)) {
	button_counter++;
	console_printf("READ_INPUT_PIN executed...");
	console_printf("COUNTER:%d\n",button_counter);
	os_timer_disarm(&input_pin_timer); // Disarm input pin timer
	os_timer_setfn(&input_pin_timer, (os_timer_func_t *)handle_pin_press, NULL); // Setup input pin timer
	os_timer_arm(&input_pin_timer, button_timeout, 1); // Arm input pin timer, 0,5sec, repeat
	sleepms(button_debounce);  // debounce time
	}
	gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status); 	//clear interrupt status

	ETS_GPIO_INTR_ENABLE(); // Enable gpio interrupts}
}

LOCAL void ICACHE_FLASH_ATTR handle_pin_press() {
console_printf("TIMER EXECUTED\n");
if(button_counter==1){
	if(!GPIO_INPUT_GET(PIN_GPIO13)){  // button is pressed once and is still pressed after the timeout
		MQTT_Publish(&mqttClient,BR_MODE,"2",1,0,0); // Set overall MODE to OFF
		GPIO_OUTPUT_SET(PIN_GPIO5, 0);
		MQTT_Publish(&mqttClient,PIN_GPIO5_TOPIC,"OFF",3,0,0);
		GPIO_OUTPUT_SET(PIN_GPIO12, 0);
		MQTT_Publish(&mqttClient,PIN_GPIO12_TOPIC,"OFF",3,0,0);
		GPIO_OUTPUT_SET(PIN_GPIO14, 0);
		MQTT_Publish(&mqttClient,PIN_GPIO14_TOPIC,"OFF",3,0,0);
		button_counter=0;
	}
}
	switch(button_counter){
	case 1:
		if(!GPIO_INPUT_GET(PIN_GPIO5)){
			console_printf("FAN STARTED BY BUTTON. Only 1 press detected\n");
			GPIO_OUTPUT_SET(PIN_GPIO5, 1);
			MQTT_Publish(&mqttClient,PIN_GPIO5_TOPIC,"ON",2,0,0);
			MQTT_Publish(&mqttClient,BR_MODE,"4",1,0,0);
		} else {
			console_printf("FAN STOPPED BY BUTTON. Only 1 press detected\n");
			GPIO_OUTPUT_SET(PIN_GPIO5, 0);
			MQTT_Publish(&mqttClient,PIN_GPIO5_TOPIC,"OFF",3,0,0);
		}
		break;
/*		GPIO_OUTPUT_SET(PIN_GPIO5, curr_input_pin_state);
		MQTT_Publish(&mqttClient,PIN_GPIO13_TOPIC,"ON",2,0,0);
		MQTT_Publish(&mqttClient,PIN_GPIO5_TOPIC,"ON",2,0,0);/**/
	case 2:
		if(!GPIO_INPUT_GET(PIN_GPIO14)){
			console_printf("BOILER STARTED BY BUTTON. 2 presses detected\n");
			GPIO_OUTPUT_SET(PIN_GPIO14, 1);
			MQTT_Publish(&mqttClient,PIN_GPIO14_TOPIC,"ON",2,0,0);
		} else {
			console_printf("BOILER STOPPED BY BUTTON. 2 presses detected\n");
			GPIO_OUTPUT_SET(PIN_GPIO14, 0);
			MQTT_Publish(&mqttClient,PIN_GPIO14_TOPIC,"OFF",3,0,0);
			MQTT_Publish(&mqttClient,BR_MODE,"2",1,0,0);
		}
		break;
/*	case 3:
		if(!GPIO_INPUT_GET(PIN_GPIO12)){
			console_printf("HEATING STARTED BY BUTTON. 3 presses detected\n");
			GPIO_OUTPUT_SET(PIN_GPIO12, 1);
			MQTT_Publish(&mqttClient,PIN_GPIO12_TOPIC,"OFF",3,0,0);
		} else {
			console_printf("HEATING STOPPED BY BUTTON. 3 presses detected\n");
			GPIO_OUTPUT_SET(PIN_GPIO12, 0);
			MQTT_Publish(&mqttClient,PIN_GPIO12_TOPIC,"OFF",3,0,0);
		}
		break;/**/
	}
	button_counter=0; // zero the counter
	os_timer_disarm(&input_pin_timer); // Disarm input pin timer
}


LOCAL void ICACHE_FLASH_ATTR sntp_cb() {
	uint32 current_stamp;
	current_stamp = sntp_get_current_timestamp();
	os_printf("sntp: %d, %s \n",current_stamp, sntp_get_real_time(current_stamp));
}

LOCAL void ICACHE_FLASH_ATTR sntp_starts_here(){
	ipaddr_aton("192.168.254.30", &NTP1);
	//	sntp_setservername(0, ”bg.pool.ntp.org”); // set server 0 by domain name
	//	sntp_setservername(1, ”us.pool.ntp.org”); // set server 0 by domain name
	sntp_setserver(0,NTP1);
	sntp_set_timezone(+2);
	sntp_init();
	os_timer_disarm(&ds18b20_timer);
	os_timer_setfn(&ds18b20_timer, (os_timer_func_t *)sntp_cb, (void *)0);
	os_timer_arm(&ds18b20_timer, DELAY, 1);

}


//Main routine. Initialize stdout, the I/O and the webserver and we're done.
void user_init(void) {
// my stuff
	dBR_BOILER_SET=65; // default setting 65C boiler temperature on reboot
	dBR_TEMP_ROOM_SET=18; // default setting 18C room temperature on reboot
	dBR_HUMIDITY_SET=60; // default setting 65% humidity target on reboot
	dBR_MODE=2; // default BR MODE is AUTO
	dDEVICE_MODE=1; // This mode defines if device uses remote (MQTT) management or uses local logics - 0 for remote, 1 for local mode. DEFAULT is 0 - remote

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
// DHT22 initialize
	DHTInit(DHT22, DELAY);
	os_timer_disarm(&dht22_timer);
	os_timer_setfn(&dht22_timer, (os_timer_func_t *)dht22_cb, (void *)0);
	os_timer_arm(&dht22_timer, DELAY, 1);
// INPUT PIN initialize
	ETS_GPIO_INTR_DISABLE(); // Disable gpio interrupts
	ETS_GPIO_INTR_ATTACH(read_input_pin, 13); // GPIO13 interrupt handler
	PIN_FUNC_SELECT(PIN_GPIO13_MUX, PIN_GPIO13_FUNC);
	gpio_output_set(0, 0, 0, BIT13); // Set GPIO13 as input
	PIN_PULLUP_EN(PIN_GPIO13_MUX); // Enable pullup
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(13)); // Clear GPIO12 status
	gpio_pin_intr_state_set(GPIO_ID_PIN(13), GPIO_PIN_INTR_NEGEDGE); // Interrupt on NEGATIVE GPIO13 edge
	ETS_GPIO_INTR_ENABLE(); // Enable gpio interrupts
//	INFO("Input pin INITIALIZED ! ! !");

// initialize GPIO12
	PIN_FUNC_SELECT(PIN_GPIO12_MUX, PIN_GPIO12_FUNC);
	GPIO_OUTPUT_SET(PIN_GPIO12, 0);
//	INFO("GPIO12 set to OFF\r\n");
// initialize GPIO5
	GPIO_OUTPUT_SET(PIN_GPIO5, 0);
//	INFO("GPIO5 set to OFF\r\n");
// initialize GPIO14
	PIN_FUNC_SELECT(PIN_GPIO14_MUX, PIN_GPIO14_FUNC);
	GPIO_OUTPUT_SET(PIN_GPIO14, 0);
//	INFO("GPIO14 set to OFF\r\n");
	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);

	os_printf("\nReady\n");
}

// http://www.esp8266.com/viewtopic.php?f=6&t=1443 -> GPIO Interrupt handling ! ! !

