#include <Arduino.h>
#include "DHT.h"
#include <DHT_U.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define DHTPIN 4
#define LEDPIN 5

#define DHTTYPE DHT11 

const char* ssid = "TP-Link_8888";
const char* password = "intreabalpelau";
const char* token = "SWMbLR5vACHCxpl9iFxG";

unsigned long lastSend = 0;
unsigned long blinkStart = 0;
bool isBlinking = false;

RTC_DATA_ATTR float lastTemp = 0.0;
RTC_DATA_ATTR float lastHumidity = 0.0;
RTC_DATA_ATTR float tempDifferences[3];
RTC_DATA_ATTR float humidityDifferences[3];
RTC_DATA_ATTR uint8_t tempDiffIndex = 0;
RTC_DATA_ATTR uint8_t humidityDiffIndex = 0;
RTC_DATA_ATTR uint32_t bootCount = 0;

WiFiClient espClient;
PubSubClient client(espClient);

DHT dht(DHTPIN, DHTTYPE);

// put function declarations here:
int myFunction(int, int);

void rpcCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  if (message.indexOf("blinkLed") != -1) {
    isBlinking = true;
    digitalWrite(LEDPIN, HIGH);
    blinkStart = millis();
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  // put your setup code here, to run once:
  Serial.begin(115200);
  dht.begin();
  WiFi.begin(ssid, password);
  client.setCallback(rpcCallback);
  pinMode(LEDPIN, OUTPUT);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  client.setServer("eu.thingsboard.cloud", 1883);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client", token, NULL)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("v1/devices/me/telemetry", "{\"message\":\"Hello from ESP32\"}");
      // ... and resubscribe
      client.subscribe("v1/devices/me/rpc/request/+");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
  
  if (millis() - lastSend > 2000) { // Send telemetry every 2 seconds
    lastSend = millis();

    float temperature = dht.readTemperature();   
    float humidity = dht.readHumidity();
    if(isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
    float tempMean = 0.0;
    float humidityMean = 0.0;
    float deltaTemp = abs(temperature - lastTemp);
    float deltaHumidity = abs(humidity - lastHumidity);

    tempDifferences[tempDiffIndex] = deltaTemp;
    humidityDifferences[humidityDiffIndex] = deltaHumidity;
    tempDiffIndex = (tempDiffIndex + 1) % 3;
    humidityDiffIndex = (humidityDiffIndex + 1) % 3;

    tempMean = (tempDifferences[0] + tempDifferences[1] + tempDifferences[2]) / 3;
    humidityMean = (humidityDifferences[0] + humidityDifferences[1] + humidityDifferences[2]) / 3;
    
    float tempThreshold = max(tempMean, 0.5f);
    float humidityThreshold = max(humidityMean, 0.5f);

    if (deltaTemp > tempThreshold || deltaHumidity > humidityThreshold) {
      String payload = "{\"temperature\":" + String(temperature) + ",\"humidity\":" + String(humidity) + "}";
      client.publish("v1/devices/me/telemetry", payload.c_str());
      digitalWrite(LEDPIN, HIGH);
      isBlinking = true;
      blinkStart = millis();
      lastTemp = temperature;
      lastHumidity = humidity;
      Serial.println(payload);
      Serial.println("Threshold temperature difference: " + String(tempThreshold));
      Serial.println("Threshold humidity difference: " + String(humidityThreshold));
    }
  }

  if(isBlinking && (millis() - blinkStart >= 1000)) {
        digitalWrite(LEDPIN, LOW);
        isBlinking = false;
      }

  client.loop();
}
