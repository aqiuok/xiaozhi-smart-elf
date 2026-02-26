//************************************************************
// this is a simple example that uses the painlessMesh library
//
// 1. sends a silly message to every node on the mesh at a random time between 1 and 5 seconds
// 2. prints anything it receives to Serial.print
//
//
//************************************************************
//这是一个台灯
#include "painlessMesh.h"

#define MESH_PREFIX "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

// use first channel of 16 channels (started from zero)
#define LEDC_CHANNEL_0 0

// use 12 bit precission for LEDC timer
#define LEDC_TIMER_12_BIT 12

// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ 5000

// fade LED PIN (replace with LED_BUILTIN constant for built-in LED)
#define LED_PIN 5
int brightness = 0;  // how bright the LED is

Scheduler userScheduler;  // to control your personal task
painlessMesh mesh;

// User stub
void sendMessage();  // Prototype so PlatformIO doesn't complain

Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
  // calculate duty, 4095 from 2 ^ 12 - 1
  uint32_t duty = (4095 / valueMax) * min(value, valueMax);

  // write duty to LEDC
  ledcWrite(channel, duty);
}

void sendMessage() {
  String msg = "Hello from node ";
  msg += mesh.getNodeId();
  mesh.sendBroadcast(msg);
  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

// Needed for painless library
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  String msgtopic = msg.substring(1, 6);
  Serial.print("msgtopic:");
  Serial.println(msgtopic);
  if (msgtopic == "node1") {
    msg = msg.substring(7);
    Serial.print("msg:");
    Serial.println(msg);
    int index1 = msg.indexOf("#");
    Serial.print("index1:");
    Serial.println(index1);
    if (index1 < 0) {
      if (msg == "on") {
        brightness = 200;
        ledcAnalogWrite(LEDC_CHANNEL_0, brightness);
      } else if (msg == "off") {
        brightness = 0;
        ledcAnalogWrite(LEDC_CHANNEL_0, brightness);
      } else if (msg == "up") {
        brightness += 50;
        if (brightness > 255) {
          brightness = 255;
        }
        ledcAnalogWrite(LEDC_CHANNEL_0, brightness);
      } else if (msg == "down") {
        brightness -= 50;
        if (brightness < 0) {
          brightness = 0;
        }
        ledcAnalogWrite(LEDC_CHANNEL_0, brightness);
      }
    } else {
      Serial.print("light1:");
      Serial.println(msg.substring(index1 + 1));
      brightness = map(msg.substring(index1 + 1).toInt(), 0, 100, 0, 255);  // how bright the LED is
      Serial.print("light2:");
      Serial.println(brightness);
      ledcAnalogWrite(LEDC_CHANNEL_0, brightness);
    }
  } else if (msgtopic == "node9") {
    msg = msg.substring(7);
    Serial.print("msg:");
    Serial.println(msg);
    if (msg == "on") {
      brightness = 200;
      ledcAnalogWrite(LEDC_CHANNEL_0, brightness);
    } else if (msg == "off") {
      brightness = 0;
      ledcAnalogWrite(LEDC_CHANNEL_0, brightness);
    }
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}
bool flage = 0;
String senmsg = "";
void setup() {
  Serial.begin(115200);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);

  // Setup timer and attach timer to a led pin
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttachPin(LED_PIN, LEDC_CHANNEL_0);
  ledcAnalogWrite(LEDC_CHANNEL_0, 0);
  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes(ERROR | STARTUP);  // set before init() so that you can see startup messages

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 6);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}

void loop() {
  // it will run the user scheduler as well
  if (digitalRead(1) == 0) {
    delay(50);
    if (digitalRead(1) == 0) {
      if (flage) {
        flage = 0;
        senmsg = "#node2#on";
        Serial.println(senmsg);
        mesh.sendBroadcast(senmsg);

      } else {
        flage = 1;
        senmsg = "#node2#off";
        Serial.println(senmsg);
        mesh.sendBroadcast(senmsg);
      }
    }
    while(!digitalRead(1));
  }
  if (digitalRead(2) == 0) {
    delay(50);
    if (digitalRead(2) == 0) {
        senmsg = "#node2#led";
        Serial.println(senmsg);
        mesh.sendBroadcast(senmsg);
    }
    while(!digitalRead(2));
  }
  mesh.update();
}
