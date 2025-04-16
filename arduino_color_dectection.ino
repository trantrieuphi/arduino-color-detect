#include <Wire.h>
#include "Adafruit_TCS34725.h"

// define gpio
const int yellowPin = 2;
const int greenPin = 3;
const int redPin = 4;

// Create TCS34725 device
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);

void setup() {
  Serial.begin(9600);
  Serial.println("TCS34725 Color Sensor Control");

  // Init sensor
  if (!tcs.begin()) {
    Serial.println("Không tìm thấy cảm biến TCS34725... kiểm tra kết nối!");
    while (1); // Dừng chương trình nếu không tìm thấy cảm biến
  }
  Serial.println("Init color sensor device successful!");

  // init gpio is output
  pinMode(yellowPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(redPin, OUTPUT);

  // init value gpio
  digitalWrite(yellowPin, LOW);
  digitalWrite(greenPin, LOW);
  digitalWrite(redPin, LOW);
}

void loop() {
  // read color rgbc value
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);

  // debug print rgbc value
  Serial.print("R: "); Serial.print(r);
  Serial.print(" G: "); Serial.print(g);
  Serial.print(" B: "); Serial.print(b);
  Serial.print(" C: "); Serial.println(c);


  if (r > 150 && g > 150 && b < 100) {
    Serial.println("Phát hiện màu vàng!");
    digitalWrite(yellowPin, HIGH);
    digitalWrite(greenPin, LOW);
    digitalWrite(redPin, LOW);
  }
  else if (g > 150 && r < 100 && b < 100) {
    Serial.println("Phát hiện màu xanh lá!");
    digitalWrite(yellowPin, LOW);
    digitalWrite(greenPin, HIGH);
    digitalWrite(redPin, LOW);
  }
  else if (r > 150 && g < 100 && b < 100) {
    Serial.println("Phát hiện màu đỏ!");
    digitalWrite(yellowPin, LOW);
    digitalWrite(greenPin, LOW);
    digitalWrite(redPin, HIGH);
  }
  else {
    digitalWrite(yellowPin, LOW);
    digitalWrite(greenPin, LOW);
    digitalWrite(redPin, LOW);
    Serial.println("Không xác định được màu!");
  }

  delay(500);
}