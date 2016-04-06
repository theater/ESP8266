#include <Adafruit_BMP085.h>

#include <DallasTemperature.h>
#include <UIPEthernet.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <dht.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <SoftReset.h>
#include <Timer.h>


#define PUBLISH_INTERVAL 60000  // Publish data each minute to MQTT broker
#define RESET 3  // Reset button - pin 3 goes to GND
#define ONE_WIRE_PIN 9  // Data wire is plugged into pin 4 on the Arduino
#define DHT_PIN 8  //DHT22
//#define MANUAL_INTERVAL 300000  // Manual function check interval will be on each 5 min
#define RESET_INTERVAL 15000  // Reset counter will be changed each 1 mins, 3 times if not connected by then - reset the duino
#define gpio digitalWrite

//int resetCounter=0;
int resetCheck;
byte server[] = { 192,168,254,30 };
unsigned long now;
unsigned long sentTime=0;

uint8_t mac[6] = {0x36,0x36,0x36,0x36,0x36,0x36};
IPAddress myIP(192,168,254,36);  //IP address of Arduino

//Timer Reset_Timer;
Timer OW_GET_Timer;

//Adafruit_BMP085 bmp;
EthernetClient ethClient;
PubSubClient MQTT_Client(server, 1883, callback, ethClient);
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);
dht DHT;

int MQTT_Connect () {
  if (!MQTT_Client.connected()) {
    if (MQTT_Client.connect("ArduinoNANO-Weather")) {
      MQTT_Client.publish("Arduino","ArduinoNANO-Weather is UP");
      MQTT_Client.subscribe("Weather_pressure_set");
//      Serial.println("Connected to MQTT\n");
      return 1;
    } else {
//       Serial.println("Error connecting to MQTT\n");
       return 0;
     }
   } else return 1;
}

void Publish_Data () {
//    Serial.println("Entered publishing...");
//    bmp.begin();
    float Temperature=sensors.getTempCByIndex(0);
//    int Altitude=int(bmp.readAltitude(SEALEVEL_PRESSURE));
//    int Pressure=bmp.readPressure()/100;
    char strConvert[10];
    sensors.requestTemperatures(); // Send the command to get temperatures from 1-wire
    DHT.read22(DHT_PIN);  //read info from DHT/    

  // BMP085
/*    if (Pressure>900 && Pressure < 1100) {    
      String tmp1=String (Pressure);    
      tmp1.toCharArray(strConvert,7);
//      Serial.println("BMP05: ");   // print AQ temperature
//     Serial.println(tmp1);                  // to serial
      MQTT_Client.publish("Weather_pressure",strConvert); // send to MQTT
    }/**/
    //OneWire DS18B20
    if(Temperature>-25&&Temperature<50) { 
       dtostrf(Temperature,1,3,strConvert);
       MQTT_Client.publish("Weather_temperature1",strConvert);  // send it to MQTT broker
    }
    // DHT22
    if(DHT.humidity>0) {
    dtostrf(DHT.humidity,2,0,strConvert);
//      Serial.println("\nHumidity DHT is: "); // print DHT22 humidity
//      Serial.println(DHT.humidity);        //to serial
      MQTT_Client.publish("Weather_humidity",strConvert); // send to MQTT
    }
}
 
void callback(char* topic, byte* payload, unsigned int length) {
// ##### this function is the main part that triggers event based on the MQTT received messages. All the important input stuff is here... 
// ####### byte* to String transformation of payload starts here
  char cPayload[30];
  for (int i=0; i<=length; i++) {
    cPayload[i]=(char)payload[i];
  }
  cPayload[length]='\0';
/*  if (String (topic) == "Weather_pressure_set" ) {
    String strPayload = String(cPayload);
    SEALEVEL_PRESSURE=atof(cPayload)*100;
  }/**/
// ####### byte* to String transformation ends here  
//  Serial.print("\n Inside callback:");
//  Serial.println(cPayload);

// #### Real work/Logics start here. Using IFs to destinguish for the different MQTT subscriptions/relays (unfortunalte string not allowed in switch operator) :(    
/**/
//  Serial.println(SEALEVEL_PRESSURE);
}
/*
int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}/**/
/*
void ResetFunction() {
  if (!MQTT_Connect()){
    resetCounter++;
    Serial.print("Not connected... Counter=");Serial.println();
  } else {
//    resetCounter=0;
  }
  if(resetCounter>=3){
       Enc28J60.init(mac);
  }
  if(resetCounter>5) {
    Serial.println("Reset counter reached maximum treshold. Resetting...");
    gpio(RESET,LOW);
  }
}/**/

void setup()
{
  digitalWrite(RESET, HIGH);
  Serial.begin(115200);
  Serial.println("Starting Weatherstation...");
//  bmp.begin();
  Ethernet.begin(mac,myIP);
  delay(15);
  MQTT_Connect();
  delay(15);
//  Reset_Timer.every(RESET_INTERVAL,ResetFunction);
  OW_GET_Timer.every(PUBLISH_INTERVAL,Publish_Data);
//MQTT_Client.subscribe("Weather_pressure_set");
}

void loop()
{
// Reset_Timer.update();
 if (MQTT_Connect()){  
    MQTT_Client.loop(); 
    OW_GET_Timer.update();
  } /**/
} 
