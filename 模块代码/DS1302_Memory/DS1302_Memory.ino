
// CONNECTIONS:
// DS1302 CLK/SCLK --> 4
// DS1302 DAT/IO -->5
// DS1302 RST/CE --> 6
// DS1302 VCC --> 3.3v - 5v
// DS1302 GND --> GND

#include <RtcDS1302.h>
#include <TM1637Display.h>
#define CLK 19
#define DIO 18

TM1637Display display(CLK, DIO);
ThreeWire myWire(5, 4, 6);  // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

#define countof(a) (sizeof(a) / sizeof(a[0]))

const char data[] = "what time is it";
bool colon = true;
void setup() {
  Serial.begin(115200);
  display.setBrightness(3);
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
}

void loop() {
  //GetDateTime函数获取RTC模块时间
  RtcDateTime now = Rtc.GetDateTime();

  //printDateTime(now);
  Serial.print("hour:");
  Serial.println(now.Hour());
  Serial.print("minute:");
  Serial.println(now.Minute());
  Serial.println((int)now.Minute());


  if (!now.IsValid()) {
    // Common Causes:
    //    1) the battery on the device is low or even missing and the power line was disconnected
    Serial.println("RTC lost confidence in the DateTime!");
  }

   colon = !colon;
  int bits = colon ? 0b01000000 : 0;
  display.showNumberDecEx((int)now.Hour() * 100 + (int)now.Minute(), bits, true, 4, 0);
  delay(500);
  colon = !colon;
  bits = colon ? 0b01000000 : 0;
  display.showNumberDecEx((int)now.Hour() * 100 + (int)now.Minute(), bits, true, 4, 0);
  delay(500);
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
}
