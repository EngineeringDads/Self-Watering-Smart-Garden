#include "DHT.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#define DHTPIN D7     // DHT sensor pin
#define levelSwitch D6 // Level switch pin


#define DHTTYPE DHT11   // DHT 11

int pump = D8;
//
int sensorPin = A0; 
int sensorValue;  
int limit = 300; 

const char* ssid = 
const char* password = 
const char* mqtt_server = 

WiFiClient espClient;
PubSubClient client(espClient);

DHT dht(DHTPIN, DHTTYPE);

unsigned long now = millis();
unsigned long lastMeasure = 0;
unsigned long currentLevel = millis();
//long currentPump = millis();
unsigned long lastLevel = 0;
unsigned long lastPump = 0;
unsigned long pumpStart;
bool pumpOn = false;
bool pumpState = false;

void wificonnect() {
  delay(10);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.hostname("Name");
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
 
  // Print the IP address
  Serial.print("IP address: ");
  Serial.print(WiFi.localIP());
}

void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

   //If a message is received on the topic room/lamp, you check if the message is either on or off. Turns the lamp GPIO according to the message
  // Publishes new temperature and humidity every 30 seconds
  if(topic=="plant/pumpon"){
      Serial.print("Changing Pump to ");
      if(messageTemp == "true"){
        digitalWrite(pump, HIGH);
        Serial.print("On");
      }
  }
   if(topic=="plant/pumpoff"){
      Serial.print("Changing Pump to ");
      if(messageTemp == "true"){
        digitalWrite(pump, LOW);
        Serial.print("Off");
      }
  }
  Serial.println();
} 

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect

    if (client.connect("ESP8266Client")) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("plant/pumpon");
      client.subscribe("plant/pumpoff");
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
 Serial.begin(115200);
 wificonnect();
 client.setServer(mqtt_server, 1883);
 client.setCallback(callback);
 
 pinMode(pump, OUTPUT);
// pinMode(rainSensor, INPUT);
 pinMode(sensorPin, OUTPUT);
 pinMode(levelSwitch, INPUT_PULLUP);
 

 dht.begin();
} 

void loop() {

 levelCheck();
 pumpCheck();
 //rainCheck();
  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP8266Client");

  now = millis();
  // Publishes new temperature and humidity every 30 seconds
  if (now - lastMeasure > 30000) {
    lastMeasure = now;
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Computes temperature values in Celsius
    float hic = dht.computeHeatIndex(t, h, false);
    static char temperatureTemp[7];
    dtostrf(hic, 6, 2, temperatureTemp);

    static char humidityTemp[7];
    dtostrf(h, 6, 2, humidityTemp);

     sensorValue = analogRead(sensorPin); 
     Serial.println("Analog Value : ");
     Serial.println(sensorValue);
     
  static char soilMoist[7];
  dtostrf(sensorValue, 6, 2, soilMoist);

    // Publishes Temperature and Humidity values
    client.publish("plant/temperature", temperatureTemp);
    client.publish("plant/humidity", humidityTemp);
    client.publish("plant/soil", soilMoist);
  
  // Show measurements at Serial monitor:
  Serial.print("   Temp DHT ==> ");
  Serial.print(t);
  Serial.print("oC  Hum DHT ==> ");
  Serial.print(h);

  

//    //get the reading from the function below and print it
//  int val = readSensor();
//  Serial.print("Digital Output: ");
//  Serial.println(val);
//
//  // Determine status of rain
//  if (val) {
//    Serial.println("Status: Clear");
//  } else {
//    Serial.println("Status: It's raining");
//  }
//
//  delay(1000);  // Take a reading every second
//  Serial.println();
  }

//  This function returns the sensor output
//int readSensor() {
//  digitalWrite(sensorPower, HIGH);  // Turn the sensor ON
//  delay(10);              // Allow power to settle
//  int val = digitalRead(sensorPin); // Read the sensor output
//  digitalWrite(sensorPower, LOW);   // Turn the sensor OFF
//  return val;             // Return the value
//}
}

void levelCheck() {

  currentLevel = millis();
  // Publishes new temperature and humidity every 30 seconds
  if (currentLevel - lastLevel > 5000) {
    lastLevel = currentLevel;
    int waterLevel = digitalRead(levelSwitch);
    if (waterLevel == HIGH) {
      Serial.println("Water Tank Empty!");
    }
    static char tankLevel[7];
    dtostrf(waterLevel, 6, 2, tankLevel);

    client.publish("plant/level", tankLevel);
    }
}

void pumpCheck() {

    unsigned long currentPump = millis();
    
    if (digitalRead(pump) == HIGH){
      pumpStart = currentPump;
      pumpOn = true;
    }

    if (pumpOn) {
    if ((unsigned long)(currentPump - lastPump) >= 90000) {
        Serial.println("Pump over time");
        digitalWrite(pump, LOW);
        pumpState = true;
        lastPump = currentPump;
        pumpOn = false;
    }
    }
}

//void rainCheck() {
// currentRain = millis();
// if (currentRain - lastRain > 5000) {
//  lastRain = currentRain;
//  int rain = digitalRead(rainSensor);
//  Serial.println(rain);
//    if (rain == HIGH) {
//      Serial.println("It's raining");
//    }
//    else {
//      Serial.println("Not raining");
//    }
//    static char rainStatus[7];
//    dtostrf(rain, 6, 2, rainStatus);
//
//    client.publish("plant/rain", rainStatus);
//    }
//    }
