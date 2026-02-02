#include <PubSubClient.h>
#include <Ethernet.h>
#include <adafruit_vl53l0x.h>

//************ GLOBAL VARIABLES **********
WiFiClient espClient;
PubSubClient mqtt(espClient);
Adafruit_VL53L0X dSensor = Adafruit_VL53L0X();

const char * ssid = "AlchemyGuest";
const char * password = "VoodooVacation5601";
const char * mqttServer = "10.1.10.115";

const unsigned char triggerDistance = 50;

const unsigned long lightUpDuration = 1000UL * 5; //5 seconds

unsigned long triggerStartTime = 0;

bool isTriggered = false;

// *************** FUNCTIONS  *******************
//MQTT & NETWORK
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to Wi-Fi");
    Serial.print("WiFi.status() = ");
    Serial.println(WiFi.status());
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client_01")) {
      Serial.println("connected");
      client.publish("MermaidsTale/Shell1", "Connected!");
      client.subscribe("MermaidsTale/#");

      // Initial publish now that we're definitely connected
      client.publish("MermaidsTale/Shell1", "Searching");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(". Trying again in 5 seconds.");
      delay(5000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++)
    message += (char)payload[i];
  Serial.print("MQTT received: ");
  Serial.print(topic);
  Serial.print(" = ");
  Serial.println(message);

  if (strcmp(topic, "Shell1/command/lightUpShell") == 0) {
    lightUpShell();
  } else if (strcmp(topic, "Shell1/command/turnOffShell") == 0) {
    turnOffShell();
  }
}

void publishStatus() {
  if (!mqtt.connected())
    return;
  String summary = "";
  mqtt.publish("Shell1/summary", summary.c_str());
}

void serverInit(){
  setup_wifi();
  mqtt.setServer(mqttServer, 1883);
  mqtt.setCallback(mqttCallback);
}

//GENERAL FUNCTIONS
void lightUpShell() {
  digitalWrite(lightPin,HIGH);
  mqtt.publish("Shell1", "LightingUp");
}

void turnOffShell() {
  digitalWrite(lightPin,LOW);
  mqtt.publish("Shell1", "TurnedOff");
}


void _init(){
  Serial.begin(9600);
  //lighPin setup
  pinMode(lightPin,OUTPUT);
  digitalWrite(lightPin,LOW);
  //distance sensor initialization
  if(dSensor.begin()){
    Serial.println("Failed to initialize VL53L0X!");
    while(1);
  }
  //establish network connection and mqtt setup
  serverInit();
}

void program(){
    if (!mqtt.connected()) {
        reconnect();
    }
    mqtt.loop();

    unsigned long currentTime = millis();

    VL53L0X_RangingMeasurementData_t measure;
    dSensor.rangingTest(&measure, false);
    
    if(isTriggered)
        return; 

    if (measure.RangeStatus != 4) {
        if (measure.RangeMilliMeter < triggerDistance) {
            Serial.println("MermaidsTale/Shell1 triggered");
            Serial.print("Distance (mm): ");
            Serial.println(measure.RangeMilliMeter);
            
            lightUpShell();
            client.loop(); // Ensure message gets sent
            delay(500);

            isTriggered = true;
            triggerStartTime = currentTime;
             
        } else {
            Serial.println("Out of range");
        }
    } else {
        Serial.println("Out of range");
    }

    delay(100);


}

// ***************** SETUP *********************
void setup() {
  _init();
}

// **************** MAIN LOOP ****************
void loop() {
  program();
}


