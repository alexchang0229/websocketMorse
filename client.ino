#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

#define keyPin 4
#define LedPin 16
#define channelPin 0

int keyState = 0;
int lastkeyState = 0;
const char *initials = "AC";
String channels[10] = {"ABC"};
bool channelButtonState, prevChannelButtonState;
int channelNumber = 0;
unsigned long messageStart = 0, lastKeyTime = 0, lastHeartBeatTime = 0;
bool isMessaging = false;
char messageOut[100];
const char *websockets_server_host = "192.168.0.36"; // Enter server adress //192.168.2.27
const uint16_t websockets_server_port = 8001;        // Enter server port
String messageIn[50];
StaticJsonDocument<200> outJSON, inJSON;

using namespace websockets;

WebsocketsClient client;
void setup()
{
  Serial.begin(115200);
  // Connect to wifi
  WiFiManager wm;
  bool res;
  res = wm.autoConnect("AutoConnectAP");
  if (!res) {
    Serial.println("Failed to connect");
    ESP.restart();
  }

  // try to connect to Websockets server
  bool connected = client.connect(websockets_server_host, websockets_server_port, "/ws");
  if (connected) {
    Serial.println("Connected!");
    outJSON["initials"] = initials;
    outJSON["channel"] = channels[channelNumber];
    outJSON["action"] = "subscribe";
    serializeJson(outJSON, messageOut);
    client.send(messageOut);
  }
  else {
    Serial.println("Not Connected!");
  }

  // run callback when messages are received
  client.onMessage([&](WebsocketsMessage message)
  {
    Serial.println(message.data());
    DeserializationError err = deserializeJson(inJSON, message.data());
    if (!err) {
      //if (!inJSON["initals"].equals(initials)) {
      bool inState = inJSON["state"];
      const char* inInitials = inJSON["initials"];
      if (inState == 1) {
        tone(12, 523);
      } else {
        noTone(12);
      }
    }
    //}
  });
}

void loop() {
  // let the websockets client check for incoming messages
  if (client.available()) {
    client.poll();
  }

  if (millis() - lastHeartBeatTime > 3 * 1000) {
    client.send("HeartBeat");
    lastHeartBeatTime = millis();
  };

  keyState = digitalRead(keyPin);
  //----------- PROCESS TELEGRAPH KEY PRESSES -----------//
  if (keyState == 1 && lastkeyState != keyState) {
    //tone(12, 523);
    outJSON["initials"] = initials;
    outJSON["state"] = true;
    outJSON["action"] = "key";
    if (isMessaging == false) {
      messageStart = millis();
      outJSON["time"] = 0;
      isMessaging = true;
    } else {
      outJSON["time"] = millis() - messageStart;
      client.send(messageOut);
    }
    serializeJson(outJSON, messageOut);
    client.send(messageOut);
    lastKeyTime = millis();
  }
  if (keyState == 0 && lastkeyState != keyState) {
    //noTone(12);
    outJSON["initials"] = initials;
    outJSON["state"] = false;
    outJSON["time"] = millis() - messageStart;
    outJSON["action"] = "key";
    serializeJson(outJSON, messageOut);
    client.send(messageOut);
  }
  lastkeyState = keyState;

  if (millis() - lastKeyTime > 5 * 1000 && isMessaging == true) {
    isMessaging = false;
  }

  channelButtonState = digitalRead(channelPin);
  if(channelButtonState == 0 && channelButtonState!=prevChannelButtonState){
    changeChannels();
  }
  prevChannelButtonState = channelButtonState;

  delay(10);
}

void changeChannels() {
    channelNumber = (channelNumber + 1) % 2  ;
    for(int i=0;i<channelNumber +1;i++){
      tone(12,523);delay(100);noTone(12);
    }
    outJSON["initials"] = initials;
    outJSON["channel"] = channels[channelNumber];
    outJSON["action"] = "subscribe";
    serializeJson(outJSON, messageOut);
    client.send(messageOut);
    Serial.print(channelNumber);
}
