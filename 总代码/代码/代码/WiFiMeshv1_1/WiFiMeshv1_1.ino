// CONNECTIONS:
// DS1302 CLK/SCLK --> 4
// DS1302 DAT/IO -->5
// DS1302 RST/CE --> 6
// DS1302 VCC --> 3.3v - 5v
// DS1302 GND --> GND

#include <WiFi.h>
#include "time.h"
#include "sntp.h"
#include <RtcDS1302.h>
#include <TM1637Display.h>
#include <Arduino.h>
#include <painlessMesh.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

#define MESH_PREFIX "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

#define STATION_SSID "ILIVE"
#define STATION_PASSWORD "012345678"

#define HOSTNAME "MQTT_Bridge"
#define ID_MQTT "00edbb47754baafd130d211d3c351158"  //用户私钥，控制台获取

const char* topicNode1_LED = "LED1002";
const char* topicNode2_LED = "LED2002";

#define CLK 0
#define DIO 1

TM1637Display display(CLK, DIO);

// const char* ssid       = "ILIVE";
// const char* password   = "012345678";

const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 8 * 3600;
const int daylightOffset_sec = 0;
int brightness;
const char* time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)
bool colon = true;
ThreeWire myWire(4, 10, 5);  // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);
#define countof(a) (sizeof(a) / sizeof(a[0]))

const char data[] = "what time is it";
String senmsg = "";
String Node1_LED_State = "";
String Node2_LED_State = "";
struct tm timeinfo;
RtcDateTime compiled;
char str_time[12];
char str_date[12];
//时间戳
uint32_t timestamp1 = 0;  //语音发送到处理器的时间戳
uint32_t timestamp2 = 0;  //处理器发送数据的时间戳
uint32_t timestamp3 = 0;  //被控设备接收到数据的时间戳
uint32_t timestamp4 = 0;  //接收到被控设备回传消息的时间戳
// Prototypes
void receivedCallback(const uint32_t& from, const String& msg);
void mqttCallback(char* topic, byte* payload, unsigned int length);

IPAddress getlocalIP();

const char* mqtt_server = "bemfa.com";  //默认，MQTT服务器
const int mqtt_server_port = 9501;      //默认，MQTT服务器
IPAddress myIP(0, 0, 0, 0);
// IPAddress mqttBroker(192, 168, 1, 1);

painlessMesh mesh;
WiFiClient wifiClient;
PubSubClient mqttClient(mqtt_server, 9501, mqttCallback, wifiClient);

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("No time available (yet)");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval* t) {
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 6, 7);
  display.setBrightness(3);
  display.showNumberDecEx(0, true);  // Expect: 0000
  analogReadResolution(12);
  //WIFIMESH
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);  // set before init() so that you can see startup messages

  // Channel set to 6. Make sure to use the same channel for your mesh and for you other
  // network (STATION_SSID)
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 6);
  mesh.onReceive(&receivedCallback);

  mesh.stationManual(STATION_SSID, STATION_PASSWORD);
  mesh.setHostname(HOSTNAME);

  // Bridge node, should (in most cases) be a root node. See [the wiki](https://gitlab.com/painlessMesh/painlessMesh/wikis/Possible-challenges-in-mesh-formation) for some background
  mesh.setRoot(true);
  // This node and all other nodes should ideally know the mesh contains a root, so call this on all nodes
  mesh.setContainsRoot(true);

  // set notification call-back function
  sntp_set_time_sync_notification_cb(timeavailable);

  /**
   * NTP server address could be aquired via DHCP,
   *
   * NOTE: This call should be made BEFORE esp32 aquires IP address via DHCP,
   * otherwise SNTP option 42 would be rejected by default.
   * NOTE: configTime() function call if made AFTER DHCP-client run
   * will OVERRIDE aquired NTP server address
   */
  sntp_servermode_dhcp(1);  // (optional)

  /**
   * This will set configured ntp servers and constant TimeZone/daylightOffset
   * should be OK if your time zone does not need to adjust daylightOffset twice a year,
   * in such a case time adjustment won't be handled automagicaly.
   */
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

  /**
   * A more convenient approach to handle TimeZones with daylightOffset 
   * would be to specify a environmnet variable with TimeZone definition including daylight adjustmnet rules.
   * A list of rules for your zone could be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
   */
  //configTzTime(time_zone, ntpServer1, ntpServer2);
  // Serial.print("compiled: ");
  // Serial.print(__DATE__);
  // Serial.println(__TIME__);

  Rtc.Begin();
  //程序下载编译时，自动把系统时间定义进结构变量中

  compiled = RtcDateTime(__DATE__, __TIME__);

  printDateTime(compiled);
  Serial.println();
  //当发现RTC时间丢失或错乱，可以为RTC重新写入时间
  if (!Rtc.IsDateTimeValid()) {
    Serial.println("RTC lost confidence in the DateTime!");
    Rtc.SetDateTime(compiled);
  }

  if (Rtc.GetIsWriteProtected()) {
    Serial.println("RTC was write protected, enabling writing now");
    Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning()) {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  }


  /* comment out on a second run to see that the info is stored long term */
  // Store something in memory on the RTC
  uint8_t count = sizeof(data);
  uint8_t written = Rtc.SetMemory((const uint8_t*)data, count);  // this includes a null terminator for the string
  if (written != count) {
    Serial.print("something didn't match, count = ");
    Serial.print(count, DEC);
    Serial.print(", written = ");
    Serial.print(written, DEC);
    Serial.println();
  }
  /* end of comment out section */
  // connect to WiFi
  Serial.printf("Connecting to %s ", STATION_SSID);
  WiFi.begin(STATION_SSID, STATION_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if (mqttClient.connect(ID_MQTT)) {
    mqttClient.subscribe(topicNode1_LED);
    mqttClient.subscribe(topicNode2_LED);
  }
  // mqttClient.subscribe(topic_LED);

  // Serial.println(" CONNECTED");
  //获取网络时间，更新DS1302中的时间

  getLocalTime(&timeinfo);

  char str_time[12];
  strftime(str_time, sizeof(str_time), "%H:%M:%S", &timeinfo);
  //strftime(str_date, sizeof(str_date), "%B %d %Y", &timeinfo);
  //Serial.println(__DATE__);
  //Serial.println(str_date);
  compiled = RtcDateTime(__DATE__, str_time);
  Rtc.SetDateTime(compiled);
}

void loop() {
  mesh.update();
  mqttClient.loop();
  if (myIP != getlocalIP()) {
    myIP = getlocalIP();
    Serial.println("My IP is " + myIP.toString());

    if (mqttClient.connect(ID_MQTT)) {
      mqttClient.subscribe(topicNode1_LED);
      mqttClient.subscribe(topicNode2_LED);
    }
  }
  if (Serial1.available()) {  // 如果有数据可用
    String hexString = "";
    while (Serial1.available() > 0) {  // 反复读取串口直到没有为止
      byte incomingByte = Serial1.read();
      hexString += (incomingByte < 16 ? "0" : "") + String(incomingByte, HEX);  // 将字节转换为字符串表示的十六进制
    }
    timestamp1 = mesh.getNodeTime();
    Serial.print(timestamp1);
    Serial.print("  Received Hex String: ");
    Serial.println(hexString);
    //开关灯
    if (hexString == "98a30b11") {  //打开台灯
      senmsg = "#node1#on";
      mesh.sendBroadcast(senmsg);
      timestamp2 = mesh.getNodeTime();
      Serial.print(timestamp2);
      Serial.print("  ");
      Serial.println(senmsg);

    } else if (hexString == "98a30b12") {  //关闭台灯
      senmsg = "#node1#off";
      mesh.sendBroadcast(senmsg);
      timestamp2 = mesh.getNodeTime();
      Serial.print(timestamp2);
      Serial.print("  ");
      Serial.println(senmsg);

    } else if (hexString == "98a30b13") {  //台灯调亮一点
      senmsg = "#node1#up";
      mesh.sendBroadcast(senmsg);
      timestamp2 = mesh.getNodeTime();
      Serial.print(timestamp2);
      Serial.print("  ");
      Serial.println(senmsg);

    } else if (hexString == "98a30b14") {  //台灯调暗一点
      senmsg = "#node1#down";
      mesh.sendBroadcast(senmsg);
      timestamp2 = mesh.getNodeTime();
      Serial.print(timestamp2);
      Serial.print("  ");
      Serial.println(senmsg);

    } else if (hexString == "98a30bff") {  //打开所有的灯
      senmsg = "#node9#on";
      mesh.sendBroadcast(senmsg);
      timestamp2 = mesh.getNodeTime();
      Serial.print(timestamp2);
      Serial.print("  ");
      Serial.println(senmsg);

    } else if (hexString == "98a30bfe") {  //关闭所有的灯
      senmsg = "#node9#off";
      mesh.sendBroadcast(senmsg);
      timestamp2 = mesh.getNodeTime();
      Serial.print(timestamp2);
      Serial.print("  ");
      Serial.println(senmsg);

    } else if (hexString == "98a30b21") {  //打开客厅灯
      senmsg = "#node2#on";
      mesh.sendBroadcast(senmsg);
      timestamp2 = mesh.getNodeTime();
      Serial.print(timestamp2);
      Serial.print("  ");
      Serial.println(senmsg);

    } else if (hexString == "98a30b22") {  //关闭客厅灯
      senmsg = "#node2#off";
      mesh.sendBroadcast(senmsg);
      timestamp2 = mesh.getNodeTime();
      Serial.print(timestamp2);
      Serial.print("  ");
      Serial.println(senmsg);
    }
  }

  int analogValue = analogRead(3);
  // Serial.println(analogValue);
  if (analogValue >= 1300)
    brightness = 1;
  else if (analogValue < 1300 && analogValue >= 1100)
    brightness = 2;
  else if (analogValue < 1100 && analogValue >= 900)
    brightness = 3;
  else if (analogValue < 900 && analogValue >= 700)
    brightness = 4;
  else if (analogValue < 700)
    brightness = 5;
  display.setBrightness(brightness);
  // Serial.println(brightness);
  //GetDateTime函数获取RTC模块时间
  RtcDateTime now = Rtc.GetDateTime();
  printDateTime(now);

  if (!now.IsValid()) {
    // Common Causes:
    //    1) the battery on the device is low or even missing and the power line was disconnected
    Serial.println("RTC lost confidence in the DateTime!");
  }
  colon = !colon;
  int bits = colon ? 0b01000000 : 0;
  display.showNumberDecEx(now.Hour() * 100 + now.Minute(), bits, true, 4, 0);
  delay(500);
  colon = !colon;
  bits = colon ? 0b01000000 : 0;
  display.showNumberDecEx(now.Hour() * 100 + now.Minute(), bits, true, 4, 0);
  delay(500);
  // if(now.Hour()==0 && now.Minute()==0 && now.Second()==0) {
  //   getLocalTime(&timeinfo);
  //   strftime(str_time, sizeof(str_time), "%H:%M:%S", &timeinfo);
  //   strftime(str_date, sizeof(str_date), "%B %d %Y", &timeinfo);
  //   compiled = RtcDateTime(str_date, str_time);
  //   Rtc.SetDateTime(compiled);
  // }
  //printLocalTime();     // it will take some time to sync time :)
}

void printDateTime(const RtcDateTime& dt) {
  char datestring[20];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second());
  // Serial.print(datestring);
  // Serial.println("");
}
void receivedCallback(const uint32_t& from, const String& msg) {
  timestamp4 = mesh.getNodeTime();
  Serial.print(timestamp4);
  Serial.print("  ");
  Serial.printf("bridge: Received from %u msg=%s\n", from, msg.c_str());
  String topic = "painlessMesh/from/" + String(from);
  mqttClient.publish(topic.c_str(), msg.c_str());
  String msgtopic = msg.substring(1, 6);
  //Serial.println(msg);
  //Serial.println(msgtopic);
  String str = msg;
  //Serial.println(str);
  str = str.substring(7);
  int index;
  index = str.indexOf("#");
  String str1 = str.substring(0, index);

  Serial.println(str1);
  //LED开关
  if (msgtopic == "node1") {
    //LED开关 str1
    if (Node1_LED_State != str1) {  //灯状态改变的时候才发送
      mqttClient.publish(topicNode1_LED, str1.c_str());
      //Serial.print("灯状态已改变：");
      //Serial.println(str1);
      Node1_LED_State = str1;
    }
  }
  //灯开关
  else if (msgtopic == "node2") {
    //LED开关 str1
    if (Node2_LED_State != str1) {  //灯状态改变的时候才发送
      mqttClient.publish(topicNode2_LED, str1.c_str());
      //Serial.print("灯状态已改变：");
      //Serial.println(str1);
      Node2_LED_State = str1;
    }
  }
  //时间戳计算
  String str_TIME = msg;
  int index_TIME = str_TIME.indexOf("#");
  String msgtopic_TIME = str_TIME.substring(0, index_TIME);
  String timestamp = str_TIME.substring(index_TIME + 1);
  // Serial.println("Data:");
  //Serial.print(msgtopic_TIME);
  //   Serial.println(" ");
  //Serial.println(timestamp3);
  if (msgtopic_TIME == "TIME") {
    //Serial.print("timestamp = ");
    //Serial.println(timestamp);
    int timestamp1 = timestamp.toInt();
    uint32_t timestamp3 = (uint32_t)timestamp1;
    //Serial.print("timestamp3 = ");
    //Serial.println(timestamp3);
    /* 
    //时间戳
uint32_t timestamp1 = 0;  //语音发送到处理器的时间戳
uint32_t timestamp2 = 0;  //控制设备发送数据的时间戳
uint32_t timestamp3 = 0;  //被控设备接收到数据的时间戳
uint32_t timestamp4 = 0;  //接收到被控设备回传消息的时间戳
    */
    //语音到被控设备发送数据的时间
    Serial.println("时间戳的差     时间差");
    uint32_t Time1 = timestamp2 - timestamp1;
    uint32_t differenceTime1 = Time1 / 1000;
    Serial.println("语音到被控设备发送数据的时间");
    Serial.print(Time1);
    Serial.print("     ");
    Serial.println(differenceTime1);
    //控制设备发送数据到被控设备的时间
    uint32_t Time2 = timestamp3 - timestamp2;
    uint32_t differenceTime2 = Time2 / 1000;
    Serial.println("控制设备发送数据到被控设备的时间");
    Serial.print(Time2);
    Serial.print("     ");
    Serial.println(differenceTime2);
    //被控设备接收到数据到控制设备接收到回调信息的时间
    uint32_t Time3 = timestamp4 - timestamp3;
    uint32_t differenceTime3 = Time3 / 1000;
    Serial.println("被控设备接收到数据到控制设备接收到回调信息的时间");
    Serial.print(Time3);
    Serial.print("     ");
    Serial.println(differenceTime3);
    //带消息确认的整个流程的时间
    uint32_t Time4 = timestamp4 - timestamp1;
    uint32_t differenceTime4 = Time4 / 1000;
    Serial.println("带消息确认的整个流程的时间");
    Serial.print(Time4);
    Serial.print("     ");
    Serial.println(differenceTime4);
  }
}

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  char* cleanPayload = (char*)malloc(length + 1);
  memcpy(cleanPayload, payload, length);
  cleanPayload[length] = '\0';
  String msg = String(cleanPayload);
  free(cleanPayload);

  String targetStr = String(topic);  //.substring(16)
  Serial.print("MQTT接收到的主题：");
  Serial.println(targetStr);
  Serial.println(msg);
  //节点一
  if (targetStr == topicNode1_LED) {
    // Serial.println(msg);
    // if(msg == "on") {
    //   digitalWrite(12,HIGH);
    // }
    // else if(msg == "off") {
    //   digitalWrite(12,LOW);
    //   Serial.println("I m o K !");
    // }
    String senmsg = "#node1#" + msg;
    Serial.println(senmsg);
    mesh.sendBroadcast(senmsg);
  }
  //节点二//灯开关#电机开关#光敏数值
  else if (targetStr == topicNode2_LED) {
    Serial.println(msg);
    if (msg == "on") {
      digitalWrite(12, HIGH);
      // LCD_LED2 = "on";
    } else if (msg == "off") {
      digitalWrite(12, LOW);
      //Serial.println("I m o K 2 !");
      // LCD_LED2 = "off";
    }
    String senmsg = "#node2#" + msg;
    Serial.println(senmsg);
    mesh.sendBroadcast(senmsg);
  }
}

IPAddress getlocalIP() {
  return IPAddress(mesh.getStationIP());
}