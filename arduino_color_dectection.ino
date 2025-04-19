#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <math.h> // Cần thư viện math cho hàm max, min, fmod

// define gpio
const int yellowPin = 2; // Đèn báo màu Vàng
const int bluePin = 3;  // Đèn báo màu Xanh lá (sẽ dùng để báo màu Xanh dương)
const int redPin = 4;    // Đèn báo màu Đỏ

// --- Định nghĩa kiểu Enum cho các màu sắc xác định ---
enum DetectedColor {
  COLOR_UNDETERMINED, // Màu không xác định
  COLOR_RED,          // Màu Đỏ
  COLOR_YELLOW,       // Màu Vàng
  COLOR_BLUE          // Màu Xanh dương
};

// Create TCS34725 device
// Sử dụng thời gian tích hợp và gain cao hơn để lấy được nhiều ánh sáng hơn
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_4X);

// Biến để lưu thời điểm lần cuối cùng đọc cảm biến
unsigned long lastReadTime = 0;
// Khoảng thời gian tối thiểu giữa các lần đọc (lớn hơn hoặc bằng integration time)
const unsigned long readInterval = 160; // milliseconds (154ms integration time + buffer)

// Sai so gia tri mau
int tolerance_H = 5;
int tolerance_S = 0.1;
int tolerance_V = 0.1;

// HSV mau do
float H_red = 170;
float S_red = 0.19;
float V_red = 0.33;

// HSV mau xanh duong
float H_blue = 195;
float S_blue = 0.54;
float V_blue = 0.4;

// HSV vang
float H_yellow = 67;
float S_yellow = 0.41;
float V_yellow = 0.37;

// color value
DetectedColor pre_color;

// Hàm chuyển đổi RGB (0-255) sang HSV (H: 0-360, S: 0-1, V: 0-1)
// r, g, b: giá trị màu đỏ, xanh lá, xanh dương (0-255)
// h, s, v: con trỏ đến biến Hue, Saturation, Value để lưu kết quả
void RGBtoHSV(float r, float g, float b, float *h, float *s, float *v) {
    r /= 255.0; // Chuẩn hóa R, G, B về khoảng 0-1
    g /= 255.0;
    b /= 255.0;

    float max_val = fmax(fmax(r, g), b);
    float min_val = fmin(fmin(r, g), b);
    float delta = max_val - min_val;

    // Tính Hue (H)
    if (delta == 0) {
        *h = 0; // Màu xám, Hue không xác định (gán 0)
    } else if (max_val == r) {
        *h = fmod(60 * ((g - b) / delta) + 360, 360);
    } else if (max_val == g) {
        *h = fmod(60 * ((b - r) / delta) + 120, 360);
    } else if (max_val == b) {
        *h = fmod(60 * ((r - g) / delta) + 240, 360);
    }

    // Tính Saturation (S)
    if (max_val == 0) {
        *s = 0;
    } else {
        *s = delta / max_val;
    }

    // Tính Value (V)
    *v = max_val;
}

// --- Function để phân loại màu từ giá trị HSV ---
// h, s, v: Giá trị Hue (0-360), Saturation (0-1), Value (0-1)
// Trả về một giá trị thuộc enum DetectedColor
DetectedColor classifyColorHSV(float h, float s, float v) {

    // --- Kiểm tra Hue để xác định màu (khi Saturation đủ cao) ---
    // Cần hiệu chuẩn các ngưỡng Hue này dựa trên màu thực tế và môi trường

    // Màu Đỏ: Hue ở gần 0 hoặc 360
    if ((h >= H_red - tolerance_H && h < H_red + tolerance_H) && (s > S_red - tolerance_S && s <= S_red + tolerance_S) && (v > V_red - tolerance_V && v <= V_red + tolerance_V)){
        return COLOR_RED;
    }
    // Màu Vàng: Hue khoảng 60
    if ((h >= H_yellow - tolerance_H && h < H_yellow + tolerance_H) && (s > S_yellow - tolerance_S && s <= S_yellow + tolerance_S) && (v > V_yellow - tolerance_V && v <= V_yellow + tolerance_V)){
        return COLOR_YELLOW;
    }
    // Màu Xanh dương: Hue khoảng 240
    if ((h >= H_blue - tolerance_H && h < H_blue + tolerance_H) && (s > S_blue - tolerance_S && s <= S_blue + tolerance_S) && (v > V_blue - tolerance_V && v <= V_blue + tolerance_V)){
        return COLOR_BLUE;
    }

    // Nếu Saturation đủ cao nhưng Hue không thuộc các dải trên
    return COLOR_UNDETERMINED;
}

void setup() {
  Serial.begin(9600);
  Serial.println("TCS34725 Color Sensor Control (Detecting Red, Yellow, Blue)");

  // Init sensor
  if (!tcs.begin()) {
    Serial.println("Không tìm thấy cảm biến TCS34725... kiểm tra kết nối!");
    while (1); // Dừng chương trình nếu không tìm thấy cảm biến
  }
  Serial.println("Init color sensor device successful!");

  // init gpio is output
  pinMode(yellowPin, OUTPUT);
  pinMode(bluePin, OUTPUT); // green pin used for Blue detection
  pinMode(redPin, OUTPUT);

  // init value gpio
  digitalWrite(yellowPin, LOW);
  digitalWrite(bluePin, LOW);
  digitalWrite(redPin, LOW);
}

void loop() {
  // Lấy thời gian hiện tại
  unsigned long currentTime = millis();
  DetectedColor color;

  // Kiểm tra xem đã đủ thời gian để đọc cảm biến lần tiếp theo chưa
  if (currentTime - lastReadTime >= readInterval) {
    // Cập nhật thời gian đọc lần cuối
    lastReadTime = currentTime;

    // read color rgb value (float 0-255 approximation)
    float r, g, b;
    tcs.getRGB(&r, &g, &b);

    // Chuyển đổi RGB sang HSV
    float h, s, v;
    RGBtoHSV(r, g, b, &h, &s, &v);

    // debug print rgb and hsv values
    Serial.print("R: "); Serial.print(r);
    Serial.print(" G: "); Serial.print(g);
    Serial.print(" B: "); Serial.print(b);
    Serial.print("   H: "); Serial.print(h);
    Serial.print(" S: "); Serial.print(s);
    Serial.print(" V: "); Serial.println(v);

    // --- Color Detection Logic (Using HSV - Adjust these thresholds) ---
    // Nhận diện chỉ Đỏ, Vàng, Xanh dương dựa trên Hue và Saturation
    // Hue (H): 0-360 độ
    // Saturation (S): 0-1
    color = classifyColorHSV(h, s, v);
    

    // Phát hiện màu Đỏ: Hue ở gần 0 hoặc 360
    if (color == COLOR_RED) {
        if(color!= pre_color){
          Serial.println("red detected!");
        }
        digitalWrite(redPin, HIGH);
        digitalWrite(yellowPin, LOW);
        digitalWrite(bluePin, LOW); // Tắt đèn báo màu Xanh dương (Green LED)
    }
    // Phát hiện màu Vàng: Hue khoảng 60
    else if (color == COLOR_YELLOW) {
        if(color!= pre_color){
          Serial.println("yellow detected!");
        }
        digitalWrite(yellowPin, HIGH);
        digitalWrite(redPin, LOW);
        digitalWrite(bluePin, LOW); // Tắt đèn báo màu Xanh dương (Green LED)
    }
     // Phát hiện màu Xanh dương: Hue khoảng 240
    else if (color == COLOR_BLUE) { // Ngưỡng ví dụ cho Xanh dương
        if(color!= pre_color){
          Serial.println("blue detected!");
        }
        digitalWrite(bluePin, HIGH); // Bật đèn Xanh lá để báo hiệu Xanh dương
        digitalWrite(yellowPin, LOW);
        digitalWrite(redPin, LOW);
    }
    else {
      if(color!= pre_color){
        Serial.println("color unknown!");
      }
      digitalWrite(yellowPin, LOW);
      digitalWrite(bluePin, LOW);
      digitalWrite(redPin, LOW);
    }
  }
  pre_color = color;
}