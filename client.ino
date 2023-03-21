#include <ArduinoWebsockets.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#define inMessageBuf_SIZE 30
#define keyPin 4
int keyState = 0;
int lastkeyState = 0;
const char *initials = "AC";
unsigned long inMessageTime, playbackStart = 0, messageStart = 0, lastKeyTime = 0, lastInMessageTime = 0;
unsigned long inMessageBuf[inMessageBuf_SIZE];
int bufInd = 0, playbackInd = 0;
bool isMessaging = false, inState;
char buf[30], messageOut[50];
const char *websockets_server_host = "192.168.0.36"; // Enter server adress //192.168.2.27
const uint16_t websockets_server_port = 8001;        // Enter server port
String messageIn[50], oprInitials;
using namespace websockets;

WebsocketsClient client;
void setup()
{
  Serial.begin(115200);
  // Connect to wifi
  WiFiManager wm;
  //wm.resetSettings();
  bool res;
  res = wm.autoConnect("AutoConnectAP");
  if (!res) {
    Serial.println("Failed to connect");
    ESP.restart();
  }

  // try to connect to Websockets server
  bool connected = client.connect(websockets_server_host, websockets_server_port, "/ws");
  if (connected)
  {
    Serial.println("Connected!");
    client.send("Hello Server");
  }
  else
  {
    Serial.println("Not Connected!");
  }

  // run callback when messages are received
  client.onMessage([&](WebsocketsMessage message)
  {
    Serial.println(message.data());
    if (message.data()[2] == 'T' || message.data()[2] == 'F') {
      oprInitials = message.data().substring(0, 2);
      //if (!oprInitials.equals(initials)) {
      if (message.data()[2] == 'T') {
        inState = true;
      } else {
        inState = false;
      }
      inMessageTime = message.data().substring(3).toInt();
      inMessageBuf[bufInd] = inMessageTime;
      bufInd = (bufInd + 1) % inMessageBuf_SIZE;
      //}
    }
  });
}

void loop() {
  // let the websockets client check for incoming messages
  if (client.available()) {
    client.poll();
  }
  if (inMessageBuf[playbackInd] >= lastInMessageTime) {
    Serial.print("STATE:");
    Serial.print(inState);
    Serial.print("|millis:");
    Serial.print(millis());
    Serial.print("|in + start:");
    Serial.println(inMessageBuf[playbackInd] + playbackStart);
    if (inMessageBuf[playbackInd] == 0 && inState == 1) {
      Serial.println("first beep");
      playbackStart = millis();
      tone(12, 523);
      lastInMessageTime = inMessageBuf[playbackInd];
      playbackInd = (playbackInd + 1) % inMessageBuf_SIZE;
    }
    if (inMessageTime != 0) {
      if (millis() >= inMessageBuf[playbackInd] + playbackStart) {
        if (inState == 1) {
          Serial.print("T:");
          Serial.println(inMessageBuf[playbackInd]);
          tone(12, 523);
        } else {
          Serial.print("F:");
          Serial.println(inMessageBuf[playbackInd]);
          noTone(12);
        }
         lastInMessageTime = inMessageBuf[playbackInd];
         playbackInd = (playbackInd + 1) % inMessageBuf_SIZE;
      }
    }
   
  }



  keyState = digitalRead(keyPin);
  //----------- PROCESS TELEGRAPH KEY PRESSES -----------//
  if (keyState == 1 && lastkeyState != keyState) {
    //tone(12, 523);
    if (isMessaging == false) {
      messageStart = millis();
      strcpy(messageOut, initials);
      strcat(messageOut, "T0");
      client.send(messageOut);
      isMessaging = true;
    } else {
      ultoa(millis() - messageStart, buf, 10);
      strcpy(messageOut, initials);
      strcat(messageOut, "T");
      strcat(messageOut, buf);
      //Serial.println(messageOut);
      client.send(messageOut);
    }
    lastKeyTime = millis();
  }
  if (keyState == 0 && lastkeyState != keyState) {
    //noTone(12);
    ultoa(millis() - messageStart, buf, 10);
    strcpy(messageOut, initials);
    strcat(messageOut, "F");
    strcat(messageOut, buf);
    //Serial.println(messageOut);
    client.send(messageOut);
  }
  lastkeyState = keyState;

  if (millis() - lastKeyTime > 5 * 1000 && isMessaging == true) {
    //Serial.println(millis() - lastKeyTime);
    isMessaging = false;
  }

  delay(3);
}
