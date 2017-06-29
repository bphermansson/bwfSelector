#include <Arduino.h>
// EspWifi
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
// Mqtt
#include <PubSubClient.h>

const char* ssid = "NETGEAR83";
const char* password = "..........";

// Mqtt topic to publish to
const char* mqtt_pub_topic = "bwfselector";
const char* mqtt_user = "emonpi";
const char* mqtt_password = "emonpimqtt2016";

WiFiClient espClient;
PubSubClient client(espClient);

// Home Assistant server
const char* server = " 192.168.1.142";  // server's address
int port = 8123;
const char* mqtt_server = "192.168.1.79";

// Boundry Wire is separated in two areas
// Each has two wires that connects to the lawnmowers base station
// The relay connects area 1, area 2 or both as one big area
// Boundry Wire Fence #1
#define relay1 5  // ESP8266-EVB built in relay
#define relay2 3
#define relayBoth 13
#define presense 14 // Nodemcu D5

// Prototypes
void setup_wifi();
boolean checkHass();
void reconnect();

boolean bstate;

void setup() {
  Serial.begin(9600);
  while(!Serial) { }

  Serial.println("Booting");
  // Presence sensor on Gpio14
  pinMode(presense, INPUT_PULLUP);

  setup_wifi();

  delay(500);
  //Serial.println(bstate);

  // Set relays depending in Hass switch
  checkHass();
  if (bstate){
    Serial.println("Switch on");
    digitalWrite(relay1, LOW);
    digitalWrite(relay2, LOW);
  }
  else {
    Serial.println("Switch off");
    digitalWrite(relay1, HIGH);
    digitalWrite(relay2, HIGH);
  }

  // Check the presence sensor and send status via Mqtt
  // Mqtt
  client.setServer(mqtt_server, 1883);
  if (!client.connected()) {
    reconnect();
  }

  int sense = digitalRead(presense);
  char buf[4];
  itoa (sense, buf, 10);
  client.publish(mqtt_pub_topic, buf);

  Serial.println("Going into deep sleep for 20 seconds");
  ESP.deepSleep(20e6); // 20e6 is 20 microseconds

}

void loop() {

}

boolean checkHass(){
  Serial.println("Connect to Hass");

  HTTPClient http;
  // Input boolean that changes depending of time of day
  // input_boolean.bwf_selector
  // Answer:
  //{"attributes": {"friendly_name": "Bwf Selector"}, "entity_id": "input_boolean.bwf_selector",
  // "last_changed": "2017-06-27T05:06:09.208409+00:00", "last_updated":
  // "2017-06-27T05:06:09.208409+00:00", "state": "off"}
  http.begin("http://192.168.1.142:8123/api/states/input_boolean.bwf_selector"); //HTTP
  int httpCode = http.GET();
  if(httpCode > 0) {
       // HTTP header has been send and Server response header has been handled
            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                Serial.println("Got data from Home Assistant");
                StaticJsonBuffer<300> jsonBuffer;
                char json[300];
                payload.toCharArray(json,300);
                JsonObject& root = jsonBuffer.parseObject(json);
                // Test if parsing succeeds.
                if (!root.success()) {
                  Serial.println("parseObject() failed");
                  //return;
                }
                String state = root["state"];
                Serial.println("The Hass switch is: ");
                Serial.println(state);

                // State is set to off at sunrise and to on when the pump runs
                if (state=="off") {
                  bstate=false;
                }
                else {
                  bstate=true;
                }
            }
        } else {
            Serial.println("[HTTP] GET... failed, error: %s\n");
           // Serial.println("http.errorToString(httpCode).c_str())";
        }
        http.end();
        return bstate;

}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  WiFi.hostname("bwfSelector");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("bwfSelector", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(mqtt_pub_topic, "Hello from bwfselector");
      // ... and resubscribe
      //client.subscribe("time"); // Time is published on the network, we use it for time keeping

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
