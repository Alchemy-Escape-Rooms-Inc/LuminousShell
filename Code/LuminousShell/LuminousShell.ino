#include <WiFi.h>
#include <PubSubClient.h>
#include "Adafruit_VL53L0X.h"
#include <FastLED.h>

#define TOTAL_LEDS 10
#define LED_PIN 4

//************ GLOBAL VARIABLES **********
WiFiClient espClient;
PubSubClient mqtt(espClient);
Adafruit_VL53L0X sensor = Adafruit_VL53L0X();

CRGB leds[TOTAL_LEDS];

const char * ssid = "AlchemyGuest";
const char * password = "VoodooVacation5601";
const char * mqttServer = "10.1.10.115";
const char * topic = "MermaidsTale/BlueShell";                  //topic for shell communication(subscription & publishing) on the broker

const unsigned char triggerDistance = 50;
const unsigned long lightUpDuration = 1000UL * 10; //10 seconds
unsigned long currentTime, startTime;

bool isTriggered = false;

// *************** FUNCTIONS  *******************
//LED STRIP
void setup_led(){
  FastLED.addLeds<WS2812B,LED_PIN,GRB>(leds, TOTAL_LEDS);
}

//TIME OF FLIGHT DISTANCE SENSOR
void setup_sensor(){
  if (!sensor.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }
}

//WIFI NETWORK
void setup_wifi() {
  delay(1000);
  Serial.println("*********** WIFI ***********");
  Serial.print("Connecting to SSID: ");
  Serial.println(ssid);

  WiFi.begin(ssid,password);

  while(WiFi.status() != WL_CONNECTED){
    delay(100);
    Serial.print("-");
  }
  Serial.println("\nConnected.");
}

//MQTT SERVER
void reconnect() {
  while (!mqtt.connected()) {
    Serial.println("******** MQTT SERVER ********");
    if (mqtt.connect("ESP32 WROOM")) {
      Serial.print("Connection to broker established: ");
      Serial.println(mqttServer);

      mqtt.publish(topic, "Connected!");        //Message sent to broker, identifying the connected shell.
      mqtt.subscribe(topic);                             //shell subscribing to the broker's topic

    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(". Trying again in 5 seconds.");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++)
    message += (char)payload[i];
  Serial.print("MQTT received: ");
  Serial.print(topic);
  Serial.print(" = ");
  Serial.println(message);

  if (strcmp(topic, "BlueShell/lightUpShell") == 0) {
    lightUpShell();
  } else if (strcmp(topic, "BlueShell/turnOffShell") == 0) {
    turnOffShell();
  } else if (strcmp(topic, "BlueShell/playSound") == 0) {
    playSound();
  } else if (strcmp(topic, "BlueShell/stopSound") == 0) {
    stopSound();
  }

}

void setup_server(){
  mqtt.setServer(mqttServer, 1883);
  mqtt.setCallback(callback);
}

//GENERAL FUNCTIONS
void lightUpShell() {
  uint8_t brightness = beatsin8(15,20,255);
  FastLED.setBrightness(brightness);
  fill_solid(leds,TOTAL_LEDS,CRGB(255,255,255));
  FastLED.show();
  mqtt.publish(topic, "Lighting up shell.");
  mqtt.loop();
  Serial.println("Lighting up the blue shell.");
}

void turnOffShell() {
  fill_solid(leds,TOTAL_LEDS,CRGB::Black);
  FastLED.show();
  mqtt.publish(topic, "TurnedOff");
  mqtt.loop();
  Serial.println("Turning off the blue shell.");
}


void playSound(){
  //depending on how the sound will be played,
  //via mqtt command,
  //wire running to speaker controller, etc...
  //add necessary code here
  //example below if publishing to another topic,
  //which will trigger play sound
  mqtt.publish(topic,"Sound1/play");
  mqtt.loop();
  Serial.println("Playing the sound.");
}

void stopSound(){
  mqtt.publish(topic,"Sound1/stop");
  mqtt.loop();
  Serial.println("Stop playing sound.");
}

void shellActivated(){
  isTriggered = true;
  lightUpShell();
  playSound();
}

void shellDeactivated(){
  isTriggered = false;
  turnOffShell();
  //stopSound();
}

void _init(){
  //FastLED setup
  setup_led();
  //ToF Distance Sensor Setup
  setup_sensor();
  //network setup
  setup_wifi();
  //mqtt setup
  setup_server();
}

void program(){
  if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop();

  if(isTriggered){
    lightUpShell();                   //must continue looping to have fade-IN (glow up) effect.
    currentTime = millis();
    if(currentTime - startTime > lightUpDuration){
      isTriggered = false;
      shellDeactivated();
    }
    delay(100);
    return;
  }

  VL53L0X_RangingMeasurementData_t measure;
  sensor.rangingTest(&measure, false);
  
  if (measure.RangeStatus != 4) {
    unsigned char distance = measure.RangeMilliMeter;
    Serial.print("Distance (mm): ");
    Serial.println(distance);

    if(triggerDistance  < distance){
      shellActivated();
      startTime = millis();
    }

  } else {
    Serial.println(" out of range ");
  }

  delay(250);
}

// ***************** SETUP *********************
void setup() {
  Serial.begin(115200);
  _init();
}

// **************** MAIN LOOP ****************
void loop() {
  program();
}


