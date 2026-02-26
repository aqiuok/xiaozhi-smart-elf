//************************************************************
// this is a simple example that uses the painlessMesh library
//
// 1. sends a silly message to every node on the mesh at a random time between 1 and 5 seconds
// 2. prints anything it receives to Serial.print
//
//
//************************************************************
#include "painlessMesh.h"

#define MESH_PREFIX "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

#define BUTTON 7  // 定义按钮引脚
#define LED 5     // 定义LED引脚
#define SWITCH 4  // 定义LED引脚

Scheduler userScheduler;  // to control your personal task
painlessMesh mesh;
String senmsg = "";
volatile bool ledState = LOW;                  // LED状态标志位
volatile unsigned long lastInterruptTime = 0;  // 记录上一次中断的时间
volatile bool isPressed = false;               // 防抖标志位

// User stub
void sendMessage();  // Prototype so PlatformIO doesn't complain

Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

void IRAM_ATTR buttonHandler() {
  unsigned long interruptTime = millis();         // 获取当前时间
  if (interruptTime - lastInterruptTime > 400) {  // 如果距离上一次中断已经超过200ms（即防抖）
    lastInterruptTime = interruptTime;            // 记录当前时间
    isPressed = true;                             // 设置防抖标志位
  }
}

void buttonTask(void *pvParameters) {
  pinMode(BUTTON, INPUT_PULLUP);                   // 初始化按钮引脚为输入模式
  attachInterrupt(BUTTON, buttonHandler, CHANGE);  // 注册中断处理函数
  while (1) {
    if (isPressed) {         // 如果有按钮被按下
      ledState = !ledState;  // 改变LED状态


      // 发送MQTT消息，向my-led-status主题发布当前LED状态
      if (ledState) {

        Serial.println("LED turned on.");
        digitalWrite(LED, ledState);
        digitalWrite(SWITCH, ledState);
      } else {

        Serial.println("LED turned off.");
        digitalWrite(LED, ledState);
        digitalWrite(SWITCH, ledState);
      }
      isPressed = false;  // 清除防抖标志位
    }
    delay(50);  // 延时以减少CPU占用率
  }
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
  if (msgtopic == "node2") {
    msg = msg.substring(7);
    Serial.print("msg:");
    Serial.println(msg);
    if (msg == "on") {
      ledState = HIGH;
      digitalWrite(LED, ledState);
      digitalWrite(SWITCH, ledState);
      Serial.println("LED turned on.");
      senmsg = "#node2#ison11";
      Serial.println(senmsg);
      mesh.sendBroadcast(senmsg);
    } else if (msg == "off") {
      ledState = LOW;
      digitalWrite(LED, ledState);
      digitalWrite(SWITCH, ledState);
      Serial.println("LED turned off.");
      senmsg = "#node2#isoff12";
      Serial.println(senmsg);
      mesh.sendBroadcast(senmsg);
    } else if (msg == "led") {
      ledState = !ledState;
      digitalWrite(LED, ledState);
      digitalWrite(SWITCH, ledState);
      Serial.println("LED turned status is " + String(ledState));
      if (ledState == 1) {
        senmsg = "#node2#ison11";
        Serial.println(senmsg);
        mesh.sendBroadcast(senmsg);
      } else if (ledState == 0) {
        senmsg = "#node2#isoff12";
        Serial.println(senmsg);
        mesh.sendBroadcast(senmsg);
      }
    }
  } else if (msgtopic == "node9") {
    msg = msg.substring(7);
    Serial.print("msg:");
    Serial.println(msg);
    if (msg == "on") {
      ledState = HIGH;
      digitalWrite(LED, ledState);
      digitalWrite(SWITCH, ledState);
      Serial.println("LED turned on.");
      senmsg = "#node2#ison11";
      Serial.println(senmsg);
      mesh.sendBroadcast(senmsg);
    } else if (msg == "off") {
      ledState = LOW;
      digitalWrite(LED, ledState);
      digitalWrite(SWITCH, ledState);
      Serial.println("LED turned off.");
      senmsg = "#node2#isoff12";
      Serial.println(senmsg);
      mesh.sendBroadcast(senmsg);
    } else if (msg == "led") {
      ledState = !ledState;
      digitalWrite(LED, ledState);
      digitalWrite(SWITCH, ledState);
      Serial.println("LED turned status is " + String(ledState));
      if (ledState == 1) {
        senmsg = "#node2#ison11";
        Serial.println(senmsg);
        mesh.sendBroadcast(senmsg);
      } else if (ledState == 0) {
        senmsg = "#node2#isoff12";
        Serial.println(senmsg);
        mesh.sendBroadcast(senmsg);
      }
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

void setup() {
  Serial.begin(115200);

  pinMode(LED, OUTPUT);     // 初始化LED引脚为输出模式
  pinMode(SWITCH, OUTPUT);  // 初始化LED引脚为输出模式
  digitalWrite(LED, LOW);
  digitalWrite(SWITCH, LOW);
  xTaskCreate(buttonTask, "Button Task", 1024 * 2, NULL, 1, NULL);  // 创建任务
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
  mesh.update();
}
