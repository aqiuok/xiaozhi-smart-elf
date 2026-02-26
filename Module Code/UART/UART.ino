void setup() {
  // 初始化串口
  Serial.begin(115200);
  Serial1.begin(9600,SERIAL_8N1, 10, 1);  // 设置串口1波特率为9600
  // 等待串口连接完成
  while (!Serial) {}
}

void loop() {

  if (Serial1.available()) {  // 如果有数据可用
    String hexString = "";
    while (Serial1.available() > 0) {  // 反复读取串口直到没有为止
      byte incomingByte = Serial1.read();
      hexString += (incomingByte < 16 ? "0" : "") + String(incomingByte, HEX);  // 将字节转换为字符串表示的十六进制
    }

    Serial.print("Received Hex String: ");
    Serial.println(hexString);
   //delay(800);
  //   // 转换为 byte 数组并输出
  //   int length = hexString.length();
  //   byte data[length / 2];
  //   for (int i = 0; i < length - 1; i += 2) {
  //     String tempString = hexString.substring(i, i + 2);
  //     byte tempByte = strtoul(tempString.c_str(), NULL, 16);
  //     data[i/2] = tempByte;
  //   }

  //   Serial.print("Received Bytes: ");
  //   for (int i = 0; i < length / 2; i++) {
  //     Serial.print(data[i], HEX);
  //     Serial.print(" ");
  //   }
  //   Serial.println();
   }
}
