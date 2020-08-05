#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "credentials.h"
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define IN  D1
#define OUT D3
#define STOP D2
#define HOSTNAME "AwningControl"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWD;

char* espTopic = "kolcun/indoor/esp";
char* controlTopic = "kolcun/outdoor/awning";
char* stateTopic = "kolcun/outdoor/awning/state";
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
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(IN, HIGH);
  digitalWrite(OUT, HIGH);
  digitalWrite(STOP, HIGH);

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
    if (strncmp(mqttMessage, "in", length - 1) == 0) {
      Serial.println("Awning In");
      closeAwning();
    } else if (strncmp(mqttMessage, "out", length - 1) == 0) {
      Serial.println("Awning out");
      openAwning();
    } else if (strncmp(mqttMessage, "stop", length - 1) == 0) {
      Serial.println("Awning stop");
      stopAwning();
    }
  }
}

void openAwning() {
  pubSubClient.publish(stateTopic, "out");
  digitalWrite(OUT, LOW);
  delay(250);
  digitalWrite(OUT, HIGH);
}

void closeAwning() {
  pubSubClient.publish(stateTopic, "in");
  digitalWrite(IN, LOW);
  delay(250);
  digitalWrite(IN, HIGH);

}

void stopAwning() {
  pubSubClient.publish(stateTopic, "stop");
  digitalWrite(STOP, LOW);
  delay(250);
  digitalWrite(STOP, HIGH);

}
