#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#define IN  D1
#define OUT D3
#define STOP D2

const char* ssid = "KrispyNet";
const char* password = "Australia";

char* espTopic = "kolcun/indoor/esp";
char* controlTopic = "kolcun/outdoor/awning";
char* stateTopic = "kolcun/outdoor/awning/state";
char* server = "52.90.29.252";//"ec2-52-90-29-252.compute-1.amazonaws.com";
char* mqttMessage;

WiFiClient wifiClient;
PubSubClient pubSubClient(wifiClient);

void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(IN, OUTPUT);
  pinMode(OUT, OUTPUT);
  pinMode(STOP, OUTPUT);
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
    if (pubSubClient.connect("mkol123", "kolcun", "MosquittoMQTTPassword$isVeryLong123")) {
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
    if (strncmp(mqttMessage, "in", length) == 0) {
      Serial.println("Awning In");
      closeAwning();
    } else if (strncmp(mqttMessage, "out", length) == 0) {
      Serial.println("Awning out");
      openAwning();
    } else if (strncmp(mqttMessage, "stop", length) == 0) {
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


