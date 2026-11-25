//TAKE NOTE: for some reason USB is only recognized EVERY OTHER PLUG IN under ubuntu. (driver issue? permissions ? ) 
// todo:  
//        ADD Current Monitor - NO PINS LEFT! - Circuit Breaker?
//        

#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <GOTStateMachine.h>
#define DEBUG  1 //1=use Serial debug 0=do not pollute the rx/tx lines.

#if DEBUG == 1
  #define debugPrint(x) Serial.print(x)
  #define debugPrintln(x) Serial.println(x) 
  #else
  #define debugPrint(x) 
  #define debugPrintln(x)
  
#endif

volatile unsigned long Counter = 0;  // Global counter variable of current position relative to index switch
volatile int FWD = 1;
volatile int HOMED = 0;
volatile int DIR = 0;

int MQTTretries = 0;
uint32_t x=0;
int MQTTPings = 0;

#define pEncoder  16 
#define pEndstop  17 
#define pFWD  18 
#define pREV  19 

#include "secrets.h"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>

// Setup a Subscription called 'positionset' for subscribing to changes on the position
Adafruit_MQTT_Subscribe positionset = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/positionset", MQTT_QOS_1);

// Setup a Publish called 'endstop' for publishing to changes to the endstop
Adafruit_MQTT_Publish endstop = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/endstop", MQTT_QOS_1);

// Setup a Publish called 'position' for publishing.
Adafruit_MQTT_Publish ENCposition = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/position");

// Setup a Publish called 'direction' for publishing.
Adafruit_MQTT_Publish ENCdirection = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/direction");

// Setup a Publish called MQTTConnections indicating Retries
Adafruit_MQTT_Publish MQTTConnections = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/MQTTConnections");


void IRAM_ATTR handleENCODER() {
  
  if (FWD){
    Counter++;  // Increment the counter on each interrupt
    }  
      else {
        Counter--;  
        }  
  debugPrint("Counter: ");
  debugPrintln(Counter);

}
void IRAM_ATTR handleENDSTOP() {
  if (!HOMED) {
    HOMED = 1;
    Counter = 0;
    //I guess this means it only needs to home once and it will never miss an encoder pulse
  }//if !homed
}

void positionsetcallback(unsigned int x) {
  debugPrint("Hey we're in a positionset callback, the position value is: ");
  debugPrintln(x);
  debugPrint(F("\nSending position value "));
  debugPrintln(x);
#if DEBUG == 0  
  // compiled out during debug because it messes with TX/RX lines
  //EXTENDING
  while (Counter < x) {
    FWD=1; // tell the callback interrupt which way we're moving
    digitalWrite( pFWD,HIGH);
    digitalWrite( pREV,LOW);
  }
  //RETRACTING
  while ((Counter > x)) {
    FWD=0; // tell the callback interrupt which way we're moving
    digitalWrite( pFWD,LOW);
    digitalWrite( pREV,HIGH);
  }
  //shut it down
  digitalWrite(pFWD,LOW);
  digitalWrite(pREV,LOW);
#endif
// Report position 
  if (! ENCposition.publish(String(Counter).c_str())) {
    debugPrintln(F("Failed"));
  } else {
    debugPrintln(F("OK!"));
  }

}//positioncallback

//publish endstop status
void endstopcallback(unsigned int x) {
  debugPrint("Hey we're in a endstop callback, the value is: ");
  debugPrintln(x);
  if (! endstop.publish(String(x++).c_str())) {
    debugPrintln(F("Failed"));
    } else {
      debugPrintln(F("OK!"));
    }//else

}//endstopcallback


void MQTT_connect(); //gotta function-prototype for functions in support.ino

void setup() {
  if (DEBUG) Serial.begin(115200);
  if (DEBUG) delay(1500);

  debugPrintln(F("Mighty Mule1"));
  debugPrintln(ESP.getFlashChipSize());
  // Connect to WiFi access point.
  debugPrint("Connecting to ");
  debugPrintln(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1500);
    debugPrint(".");
  }

  debugPrintln("WiFi connected");
  debugPrintln("IP address: "); 
  debugPrintln(WiFi.localIP());
#if DEBUG == 1
  listNetworks();
#endif

  pinMode(pEndstop,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pEndstop), handleENDSTOP, FALLING);  // Attach interrupt on falling edge (low pulse)
  
  pinMode(pEncoder, INPUT_PULLUP);  // Set GPIO0 as input with pull-up
  attachInterrupt(digitalPinToInterrupt(pEncoder), handleENCODER, FALLING);  // Attach interrupt on falling edge (low pulse)
  
#if DEBUG == 0 //No Serial output if this happens!
  pinMode(pFWD,OUTPUT);
  pinMode(pREV,OUTPUT);
  digitalWrite(pFWD,LOW);
  digitalWrite(pREV,LOW);
#endif

  //positionset.setCallback(positionsetcallback);
  mqtt.subscribe(&positionset);
  ENCposition.publish(String(0).c_str());
  endstop.publish(String(0).c_str());
  MQTTConnections.publish(String(0).c_str());
  ENCdirection.publish(String(0).c_str());
}//setup


void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();
  delay(50);
  debugPrint("Publishing HOMED ");
  if (! endstop.publish(String(HOMED).c_str())) {
    debugPrintln(F("Failed"));
  } else {
    debugPrintln(F("OK!"));
  }

  debugPrint("Publishing Position ");
  if (! ENCposition.publish(String(Counter).c_str())) {
    debugPrintln(F("Failed"));
  } else {
    debugPrintln(F("OK!"));
  }

  debugPrint("Publishing Direction ");
  if (! ENCdirection.publish(String(DIR).c_str())) {
    debugPrintln(F("Failed"));
  } else {
    debugPrintln(F("OK!"));
  }

  // this is our 'wait for incoming subscription packets and callback em' busy subloop
  // try to spend your time here:
  mqtt.processPackets(10000);
  
  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  
  if(! mqtt.ping()) { // if we can't ping 3 times, then reset
    if (MQTTPings > 3) mqtt.disconnect(); 
    MQTTPings++;// this protection seems to eliminate failure!
  } else MQTTPings = 0;
}//loop
