// Library imports and network creditals
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>
#include "secrets.h"

// Defining the pins and intervals
#define MOISTURE_SENSOR_PIN A0
#define RELAY_PIN 2
#define MOISTURE_THRESHOLD 400
#define WATERING_DURATION 3000 
#define CHECK_INTERVAL 10000 
#define MQTT_TIMEOUT 30000 

// Stores the WiFi and MQTT credentials from secrets.h
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
const char* mqtt_server = MQTT_SERVER; 

// Initializes the WiFi and MQTT clients
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

bool autoMode = true; //used to switch between auto an manual modes
bool pumpState = false; // if the pump is on or off
bool mqttEnabled = true; // if arduino is connected to MQTT server
unsigned long lastCheckTime = 0; //used to make sure we only check moistuire level ever 10 seconds

void setup() {
    pinMode(MOISTURE_SENSOR_PIN, INPUT); //declares sensor pin and it is reciving data from this pin
    pinMode(RELAY_PIN, OUTPUT); //relay pin and we will be outputting signals (High, Low)
    digitalWrite(RELAY_PIN, LOW); //Set relay pin to low because we wont the pump to start off
    
    Serial.begin(9600); //Start the serial communication with sensor and relay
    while (!Serial); //run till we have a serial connection
    
    //Connects to wifi and MQTT server
    connectWiFi();
    connectMQTT();
}

//This function is called when we have not connected to wifi and will try again intill timeout
//If time out is reached program will run locally on auto mode just from arduino
void connectWiFi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(800);
        Serial.print(".");
        if (millis() - startAttemptTime > MQTT_TIMEOUT) {
            Serial.println("Failed to connect to WiFi, switching to local auto mode.");
            mqttEnabled = false; // Disable further MQTT connection attempts
            return;
        }
    }
    Serial.println("Connected to WiFi");
}

//Used if connection to MQTT did not work first time will try intill timeout
//If time out is reached program will run locally on auto mode just from arduino
void connectMQTT() {
    if (!mqttEnabled) return; // Skip connection attempts if MQTT is disabled
    Serial.print("Connecting to the MQTT Server");
    mqttClient.setId("ArduinoNano33");
    mqttClient.onMessage(onMqttMessage);
    
    unsigned long startAttemptTime = millis();
    while (!mqttClient.connect(mqtt_server, 1883)) {
        Serial.print(".");
        delay(2000);
        
        if (millis() - startAttemptTime > MQTT_TIMEOUT) {
            Serial.println("Failed to connect to MQTT Server, switching to local auto mode");
            autoMode = true;
            mqttEnabled = false; // Disable further MQTT connection attempts
            return;
        }
    }
    Serial.println("Connected to MQTT Server");
    //This subscribes the ardunino to the mqtt topics so we can publish data to these topics
    mqttClient.subscribe("plant/water");
    mqttClient.subscribe("plant/mode");
}

//This function is responisble for reciving information from the MQTT Server
//Based on the information it will execute an action
void onMqttMessage(int messageSize) {

    String message = "";

    while (mqttClient.available()) {
        message += (char)mqttClient.read();
    }
    
    Serial.println("MQTT Message: " + message);
    
    if (message == "ON" && !autoMode) {
        checkAndWaterPlant();
    }

    else if (message == "OFF") {
        digitalWrite(RELAY_PIN, LOW);
        pumpState = false;
    } 

    else if (message == "AUTO") {
        autoMode = true;
        Serial.println("Switched to Automatic Mode");
    } 

    else if (message == "MANUAL") {
        autoMode = false;
        digitalWrite(RELAY_PIN, LOW); // Ensures the pump is turned off when switching to manual mode
        pumpState = false;
        Serial.println("Switched to Manual Mode");
    }
}

//This function is responsible for making sure the pump is not turned on if the soil is to moist
void checkAndWaterPlant() {
    int moisture = analogRead(MOISTURE_SENSOR_PIN);
    if (moisture > MOISTURE_THRESHOLD) {
        Serial.println("Watering plant...");
        digitalWrite(RELAY_PIN, HIGH);
        delay(WATERING_DURATION);
        digitalWrite(RELAY_PIN, LOW);
        Serial.println("Watering complete.");
    } 
    else Serial.println("Moisture level too high, not watering.");
  
}

//Main loop which will run continously and call the functions
void loop() {

    if (WiFi.status() != WL_CONNECTED && mqttEnabled)  connectWiFi();

    if (mqttEnabled && !mqttClient.connected()) connectMQTT();
    
    //makes sure mqtt is still connected before sending data
    mqttClient.poll();
    
    unsigned long currentTime = millis();
    
    //This is used to send information on the moisture and environment every 10 seconds to MQTT server
    if (currentTime - lastCheckTime >= CHECK_INTERVAL) {
        lastCheckTime = currentTime;
        int moisture = analogRead(MOISTURE_SENSOR_PIN);
        if (mqttEnabled && mqttClient.connected()) {
            mqttClient.beginMessage("plant/moisture");
            mqttClient.print(moisture);
            mqttClient.endMessage();
        }
        
        //This will run for example if we are running locally or auto mode is selected on GUI
        if (autoMode) checkAndWaterPlant();
        
    }
}