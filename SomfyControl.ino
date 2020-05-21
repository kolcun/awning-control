#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "credentials.h"

#define IN  D1
#define OUT D3
#define STOP D2

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

  pinMode(IN, OUTPUT);
  pinMode(OUT, OUTPUT);
  pinMode(STOP, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT); 
  digitalWrite(IN, HIGH);
  digitalWrite(OUT, HIGH);
  digitalWrite(STOP, HIGH);

  connectToWifi();

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

  if (!pubSubClient.connected()) {
    reconnect();
  }
  pubSubClient.loop();
}

void connectToWifi() {
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  blink3Times();
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
    if (strncmp(mqttMessage, "in", length-1) == 0) {
      Serial.println("Awning In");
      closeAwning();
    } else if (strncmp(mqttMessage, "out", length-1) == 0) {
      Serial.println("Awning out");
      openAwning();
    } else if (strncmp(mqttMessage, "stop", length-1) == 0) {
      Serial.println("Awning stop");
      stopAwning();
    }
  }
  blinkMessageArrived();

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

void blinkMessageArrived() {
  turnOnLed();
  delay(100);
  turnOffLed();
}

void blink3Times() {
  for (int i = 0; i < 3; i++) {
    turnOnLed();
    delay(500);
    turnOffLed();
    delay(500);
  }
  delay(2000);
}

void turnOnLed() {
  Serial.println("led on");
  digitalWrite(LED_BUILTIN, LOW);
}

void turnOffLed() {
  Serial.println("led off");
  digitalWrite(LED_BUILTIN, HIGH);
}
