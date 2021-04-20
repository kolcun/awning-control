#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "credentials.h"
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define IN  D1
#define OUT D3
#define STOP D2
#define PERGOLAIN  D5
#define PERGOLAOUT D7
#define PERGOLASTOP D6

#define HOSTNAME "AwningControl"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWD;

char* espTopic = "kolcun/indoor/esp";
char* controlTopic = "kolcun/outdoor/awning";
char* stateTopic = "kolcun/outdoor/awning/state";
char* pergolaControlTopic = "kolcun/outdoor/pergolascreen";
char* pergolaStateTopic = "kolcun/outdoor/pergolascreen/state";

char* server = MQTT_SERVER;
char* mqttMessage;

WiFiClient wifiClient;
PubSubClient pubSubClient(wifiClient);

void setup() {
  Serial.begin(115200);
  delay(10);
  setupOTA();

  pinMode(IN, OUTPUT);
  pinMode(OUT, OUTPUT);
  pinMode(STOP, OUTPUT);
  pinMode(PERGOLAIN, OUTPUT);
  pinMode(PERGOLAOUT, OUTPUT);
  pinMode(PERGOLASTOP, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(IN, HIGH);
  digitalWrite(OUT, HIGH);
  digitalWrite(STOP, HIGH);
  digitalWrite(PERGOLAIN, HIGH);
  digitalWrite(PERGOLAOUT, HIGH);
  digitalWrite(PERGOLASTOP, HIGH);

  pubSubClient.setServer(server, 1883);
  pubSubClient.setCallback(mqttCallback);

  if (!pubSubClient.connected()) {
    reconnect();
  }
}

void reconnect() {
  while (!pubSubClient.connected()) {
    if (pubSubClient.connect("somfycontroller", MQTT_USER, MQTT_PASSWD)) {
      Serial.println("Connected to MQTT broker");
      pubSubClient.publish(espTopic, "somfy control online");
      if (!pubSubClient.subscribe(controlTopic, 1)) {
        Serial.println("MQTT: unable to subscribe");
      }
      if (!pubSubClient.subscribe(pergolaControlTopic, 1)) {
        Serial.println("MQTT: unable to subscribe");
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(pubSubClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

}

void loop() {
  ArduinoOTA.handle();
  if (!pubSubClient.connected()) {
    reconnect();
  }
  pubSubClient.loop();
}


void setupOTA() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Wifi Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.setHostname(HOSTNAME);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print(F("MQTT Message Arrived ["));
  Serial.print(topic);
  Serial.print(F("] "));
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  mqttMessage = (char*) payload;

  if (strcmp(topic, controlTopic) == 0) {
    if (strncmp(mqttMessage, "UP", length - 1) == 0) {
      Serial.println("Awning closing");
      closeAwning();
    } else if (strncmp(mqttMessage, "DOWN", length - 1) == 0) {
      Serial.println("Awning opening");
      openAwning();
    } else if (strncmp(mqttMessage, "STOP", length - 1) == 0) {
      Serial.println("Awning stop / setpoint");
      stopAwning();
    }
  }
  if (strcmp(topic, pergolaControlTopic) == 0) {
    if (strncmp(mqttMessage, "UP", length - 1) == 0) {
      Serial.println("Pergola screen closing");
      closePergolaScreen();
    } else if (strncmp(mqttMessage, "DOWN", length - 1) == 0) {
      Serial.println("Pergola screen opening");
      openPergolaScreen();
    } else if (strncmp(mqttMessage, "STOP", length - 1) == 0) {
      Serial.println("Pergola screen stop / setpoint");
      stopPergolaScreen();
    }
  }
}

void openAwning() { //openhab rollershutter down == 100, downward arrow
  pubSubClient.publish(stateTopic, "DOWN");
  digitalWrite(OUT, LOW);
  delay(250);
  digitalWrite(OUT, HIGH);
}

void closeAwning() { //openhab rollershutter up == 0, upward arrow
  pubSubClient.publish(stateTopic, "UP");
  digitalWrite(IN, LOW);
  delay(250);
  digitalWrite(IN, HIGH);

}

void stopAwning() {
  pubSubClient.publish(stateTopic, "STOP");
  digitalWrite(STOP, LOW);
  delay(250);
  digitalWrite(STOP, HIGH);
}

void openPergolaScreen() { //openhab rollershutter down == 100, downward arrow
  pubSubClient.publish(pergolaStateTopic, "DOWN");
  digitalWrite(PERGOLAOUT, LOW);
  delay(250);
  digitalWrite(PERGOLAOUT, HIGH);
}

void closePergolaScreen() { //openhab rollershutter up == 0, upward arrow
  pubSubClient.publish(pergolaStateTopic, "UP");
  digitalWrite(PERGOLAIN, LOW);
  delay(250);
  digitalWrite(PERGOLAIN, HIGH);
}

void stopPergolaScreen() {
  pubSubClient.publish(pergolaStateTopic, "STOP");
  digitalWrite(PERGOLASTOP, LOW);
  delay(250);
  digitalWrite(PERGOLASTOP, HIGH);
}
