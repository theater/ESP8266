/*
Some random cgi routines. Used in the LED example and the page that returns the entire
flash as a binary. Also handles the hit counter on the main page.
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include <string.h>
#include <osapi.h>
#include "user_interface.h"
#include "mem.h"
#include "httpd.h"
#include "cgi.h"
#include "io.h"
#include <ip_addr.h>
#include "espmissingincludes.h"
#include "mqtt.h"
#include "user_config.h"
#include "gpio.h"


//cause I can't be bothered to write an ioGetLed()
static char currLedState=0;


//Cgi that turns the LED on or off according to the 'led' param in the POST data
int ICACHE_FLASH_ATTR cgiLed(HttpdConnData *connData) {
	int len;
	char buff[1024];
	
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len=httpdFindArg(connData->postBuff, "led", buff, sizeof(buff));
	if (len!=0) {
		currLedState=atoi(buff);
		ioLed(currLedState);
	}

	httpdRedirect(connData, "led.tpl");
	return HTTPD_CGI_DONE;
}


//Template code for the led page.
void ICACHE_FLASH_ATTR tplLed(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;

	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "ledstate")==0) {
		if (currLedState) {
			os_strcpy(buff, "on");
		} else {
			os_strcpy(buff, "off");
		}
	}
	httpdSend(connData, buff, -1);
}

static long hitCounter=0;

//Template code for the counter on the index page.
void ICACHE_FLASH_ATTR tplCounter(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;

	if (os_strcmp(token, "counter")==0) {
		hitCounter++;
		os_sprintf(buff, "%ld", hitCounter);
	}
	httpdSend(connData, buff, -1);
}


//Cgi that reads the SPI flash. Assumes 512KByte flash.
int ICACHE_FLASH_ATTR cgiReadFlash(HttpdConnData *connData) {
	int *pos=(int *)&connData->cgiData;
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	if (*pos==0) {
		os_printf("Start flash download.\n");
		httpdStartResponse(connData, 200);
		httpdHeader(connData, "Content-Type", "application/bin");
		httpdEndHeaders(connData);
		*pos=0x40200000;
		return HTTPD_CGI_MORE;
	}
	//Send 1K of flash per call. We will get called again if we haven't sent 512K yet.
	espconn_sent(connData->conn, (uint8 *)(*pos), 1024);
	*pos+=1024;
	if (*pos>=0x40200000+(512*1024)) return HTTPD_CGI_DONE; else return HTTPD_CGI_MORE;
}

// ===============================================================================
//     MY BULSHIT STARTS HERE
// ===============================================================================
int ICACHE_FLASH_ATTR cgiHEATER(HttpdConnData *connData) {
	int len,gpio2,gpio4;
		char buff[1024];
		if (connData->conn==NULL) {
			//Connection aborted. Clean up.
			return HTTPD_CGI_DONE;
		}
		len=httpdFindArg(connData->postBuff, "HEATER", buff, sizeof(buff));
		if (len!=0) {
			// Control of HEATER from web interface here
			if(os_strcmp(buff, "ON")==0) {
				currGPIO12State=1;
				GPIO_OUTPUT_SET(PIN_GPIO12, 1);
				INFO("HEATER set to ON\r\n");
				MQTT_Publish(&mqttClient,PIN_GPIO12_TOPIC_CB,"ON",2,0,0);
			} else {
				currGPIO12State=0;
				GPIO_OUTPUT_SET(PIN_GPIO12, 0);
				INFO("GPIO12 set to OFF\r\n");
				MQTT_Publish(&mqttClient,PIN_GPIO12_TOPIC_CB,"OFF",3,0,0);
			}
		}

		httpdRedirect(connData, "heater.tpl");
		return HTTPD_CGI_DONE;
	}

int ICACHE_FLASH_ATTR cgiFAN(HttpdConnData *connData) {
	int len;
		char buff[1024];
		if (connData->conn==NULL) {
			//Connection aborted. Clean up.
			return HTTPD_CGI_DONE;
		}
		len=httpdFindArg(connData->postBuff, "FAN", buff, sizeof(buff));
		if (len!=0) {
			if(os_strcmp(buff, "ON")==0) {
				currGPIO5State=1;
				GPIO_OUTPUT_SET(PIN_GPIO5, 1);
				INFO("FAN set to ON\r\n");
				MQTT_Publish(&mqttClient,PIN_GPIO5_TOPIC_CB,"ON",2,0,0);
			} else {
				currGPIO5State=0;
				GPIO_OUTPUT_SET(PIN_GPIO12, 0);
				INFO("FAN set to OFF\r\n");
				MQTT_Publish(&mqttClient,PIN_GPIO12_TOPIC_CB,"OFF",3,0,0);
			}
		}
		httpdRedirect(connData, "fan.tpl");
		return HTTPD_CGI_DONE;
	}

int ICACHE_FLASH_ATTR cgiBOILER(HttpdConnData *connData) {
	int len;
		char buff[1024];
		if (connData->conn==NULL) {
			//Connection aborted. Clean up.
			return HTTPD_CGI_DONE;
		}
		len=httpdFindArg(connData->postBuff, "BOILER", buff, sizeof(buff));
		if (len!=0) {
			if(os_strcmp(buff, "ON")==0) {
				currGPIO14State=1;
				GPIO_OUTPUT_SET(PIN_GPIO14, 1);
				INFO("BOILER set to ON\r\n");
				MQTT_Publish(&mqttClient,PIN_GPIO14_TOPIC_CB,"ON",2,0,0);
			} else {
				currGPIO14State=0;
				GPIO_OUTPUT_SET(PIN_GPIO14, 0);
				INFO("BOILER set to OFF\r\n");
				MQTT_Publish(&mqttClient,PIN_GPIO14_TOPIC_CB,"OFF",3,0,0);
			}
		}
		httpdRedirect(connData, "boiler.tpl");
		return HTTPD_CGI_DONE;
	}


void ICACHE_FLASH_ATTR tplHEATER(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;
//	currMqttState=GPIO_INPUT_GET(PIN_GPIO);
	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "HEATER_status")==0) {
		if (GPIO_INPUT_GET(PIN_GPIO12)) {
				os_strcpy(buff, "ON");
			} else {
			os_strcpy(buff, "OFF");
		}
	}

	httpdSend(connData, buff, -1);
}

void ICACHE_FLASH_ATTR tplFAN(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;
//	currMqttState=GPIO_INPUT_GET(PIN_GPIO);
	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "FAN_status")==0) {
		if (GPIO_INPUT_GET(PIN_GPIO5)) {
				os_strcpy(buff, "ON");
			} else {
			os_strcpy(buff, "OFF");
		}
	}
	httpdSend(connData, buff, -1);
}

void ICACHE_FLASH_ATTR tplBOILER(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;
//	currMqttState=GPIO_INPUT_GET(PIN_GPIO);
	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "BOILER_status")==0) {
		if (GPIO_INPUT_GET(PIN_GPIO14)) {
				os_strcpy(buff, "ON");
			} else {
			os_strcpy(buff, "OFF");
		}
	}
	httpdSend(connData, buff, -1);
}

void ICACHE_FLASH_ATTR tplIndex(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return;

	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "BoilerTemperature")==0) {
		os_sprintf(buff, "%s °C",DS18B20_Temperature);
	}
	if (os_strcmp(token, "Humidity")==0) {
		os_sprintf(buff, "%s %%",DHT22_Humidity);
	}
	if (os_strcmp(token, "BathTemperature")==0) {
		os_sprintf(buff, "%s °C",DHT22_Temperature);
	}
	if (os_strcmp(token, "FAN")==0) {
		if (GPIO_INPUT_GET(PIN_GPIO5)) {
			os_strcpy(buff, "ON");
		} else {
			os_strcpy(buff, "OFF");
		}
	}
	if (os_strcmp(token, "FAN_INPUT")==0) {
			if (!GPIO_INPUT_GET(PIN_GPIO13)) {
				os_strcpy(buff, "ON");
			} else {
				os_strcpy(buff, "OFF");
			}
		}
	if (os_strcmp(token, "BOILER")==0) {
		if (GPIO_INPUT_GET(PIN_GPIO14)) {
			os_strcpy(buff, "ON");
		} else {
			os_strcpy(buff, "OFF");
		}
	}
	if (os_strcmp(token, "HEATER")==0) {
		if (GPIO_INPUT_GET(PIN_GPIO12)) {
			os_strcpy(buff, "ON");
		} else {
			os_strcpy(buff, "OFF");
		}
	}
	if (os_strcmp(token, "MODE")==0) {
			if (!dDEVICE_MODE) {
				os_strcpy(buff, "REMOTE");
			} else {
				os_strcpy(buff, "LOCAL");
			}
		}
//	espconn_sent(connData->conn, (uint8 *)buff, os_strlen(buff));
	httpdSend(connData, buff, -1);
}
