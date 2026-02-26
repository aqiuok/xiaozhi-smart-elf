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

#define CLK 19
#define DIO 18

TM1637Display display(CLK, DIO);

const char* ssid       = "ILIVE";
const char* password   = "012345678";

const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long  gmtOffset_sec = 7*3600;
const int   daylightOffset_sec = 3600;
int brightness;
const char* time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)
bool colon = true;
ThreeWire myWire(5, 4, 6);  // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);
#define countof(a) (sizeof(a) / sizeof(a[0]))

const char data[] = "what time is it";

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("No time available (yet)");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
}

void setup()
{
  Serial.begin(115200);
  display.setBrightness(3);
  analogReadResolution(12);
  // set notification call-back function
  sntp_set_time_sync_notification_cb( timeavailable );

  /**
   * NTP server address could be aquired via DHCP,
   *
   * NOTE: This call should be made BEFORE esp32 aquires IP address via DHCP,
   * otherwise SNTP option 42 would be rejected by default.
   * NOTE: configTime() function call if made AFTER DHCP-client run
   * will OVERRIDE aquired NTP server address
   */
  sntp_servermode_dhcp(1);    // (optional)

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

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  
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
  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
//获取网络时间，更新DS1302中的时间
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  
  char str_time[12];
  strftime(str_time, sizeof(str_time), "%H:%M:%S", &timeinfo);
  compiled = RtcDateTime(__DATE__, str_time);
  Rtc.SetDateTime(compiled);
}

void loop()
{
  int analogValue = analogRead(0);
  Serial.println(analogValue);
  if(analogValue >= 3800)
    brightness = 1;
  else if (analogValue <3800 && analogValue >=1600)
    brightness = 2;
  else if(analogValue < 1550 && analogValue >=1200)
    brightness = 3;
  else if(analogValue < 1200 && analogValue >=850)
    brightness = 4;
  else if(analogValue < 850)
    brightness = 5;
  display.setBrightness(brightness);
  Serial.println(brightness);
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
  Serial.print(datestring);
  Serial.println("");
}