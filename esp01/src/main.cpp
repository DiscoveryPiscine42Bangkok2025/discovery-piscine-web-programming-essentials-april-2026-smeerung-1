#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ==================== สีกำหนดเอง (Modern Color Palette) ====================
#define COLOR_BG          0x10A2  // สีพื้นหลังดำอมเทาลึก
#define COLOR_CARD_BG     0x2126  // สีการ์ดเทาเข้ม
#define COLOR_CARD_BORDER 0x39E7  // สีขอบการ์ดเทาอ่อน
#define COLOR_HEADER      0x1965  // สีแถบ Header ด้านบน
#define COLOR_TEXT_MUTED  0x9255  // สีข้อความจางๆ (Sub-label)

// สีเฉพาะเซ็นเซอร์
#define COLOR_TEMP        0xFA60  // สีส้มอมแดง (Temperature)
#define COLOR_PH          0x367F  // สีฟ้าสด (pH)
#define COLOR_TDS         0xAFE5  // สีม่วงนีออน (TDS)
#define COLOR_TURB        0x07E0  // สีเขียวนีออน (Turbidity)
#define COLOR_GAS         0xFCE0  // สีเหลืองอำพัน (Gas)

// โครงสร้างข้อมูล ESP-NOW (ต้องตรงกับตัวส่ง 100%)
typedef struct struct_message {
    float temp;
    float ph;
    float tds;
    float turbidity;
    int gas;
} struct_message;

struct_message sensorData;
TFT_eSPI tft = TFT_eSPI();

bool ledState = false;

// ฟังก์ชันวาดการ์ดแบบมีมิติ
void drawCard(int x, int y, int w, int h, uint16_t borderAccent, const char* title) {
  tft.fillRoundRect(x, y, w, h, 6, COLOR_CARD_BG);
  tft.drawRoundRect(x, y, w, h, 6, COLOR_CARD_BORDER);
  tft.fillRoundRect(x + 2, y + 4, 4, h - 8, 2, borderAccent);

  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  tft.setTextSize(1);
  tft.setCursor(x + 10, y + 6);
  tft.print(title);
}

// ฟังก์ชันวาด Layout หน้าจอ
void drawModernUI() {
  tft.fillScreen(COLOR_BG);

  tft.fillRect(0, 0, 240, 30, COLOR_HEADER);
  tft.drawFastHLine(0, 30, 240, COLOR_CARD_BORDER);
  
  tft.setTextColor(TFT_WHITE, COLOR_HEADER);
  tft.setTextSize(2);
  tft.setCursor(10, 7);
  tft.print("WATER & AIR");

  drawCard(6, 36, 111, 70, COLOR_TEMP, "TEMP (*C)");
  drawCard(123, 36, 111, 70, COLOR_PH, "pH BAL");
  drawCard(6, 112, 111, 70, COLOR_TDS, "TDS (PPM)");
  drawCard(123, 112, 111, 70, COLOR_TURB, "TURBIDITY");
  drawCard(6, 188, 228, 125, COLOR_GAS, "AIR GAS QUALITY");
}

// ฟังก์ชันอัปเดตตัวเลขบนหน้าจอ
void updateDisplay() {
  char buff[16];

  // Live Indicator มุมขวาบน
  ledState = !ledState;
  tft.fillCircle(225, 15, 4, ledState ? TFT_GREEN : COLOR_HEADER);

  // 1. Temperature
  snprintf(buff, sizeof(buff), "%5.1f", sensorData.temp);
  tft.setTextColor(TFT_WHITE, COLOR_CARD_BG);
  tft.setTextSize(2);
  tft.setCursor(15, 68);
  tft.print(buff);
  tft.setTextColor(COLOR_TEMP, COLOR_CARD_BG);
  tft.print("C");

  // 2. pH Level
  snprintf(buff, sizeof(buff), "%5.2f", sensorData.ph);
  tft.setTextColor(TFT_WHITE, COLOR_CARD_BG);
  tft.setTextSize(2);
  tft.setCursor(130, 68);
  tft.print(buff);

  // 3. TDS
  snprintf(buff, sizeof(buff), "%6.0f", sensorData.tds);
  tft.setTextColor(TFT_WHITE, COLOR_CARD_BG);
  tft.setTextSize(2);
  tft.setCursor(12, 144);
  tft.print(buff);

  // 4. Turbidity
  snprintf(buff, sizeof(buff), "%5.2f", sensorData.turbidity);
  tft.setTextColor(TFT_WHITE, COLOR_CARD_BG);
  tft.setTextSize(2);
  tft.setCursor(130, 144);
  tft.print(buff);

  // 5. Gas
  if (sensorData.gas < -9999 || sensorData.gas > 99999) {
    snprintf(buff, sizeof(buff), "  ERR  ");
  } else {
    snprintf(buff, sizeof(buff), "%6d", sensorData.gas);
  }
  
  tft.setTextColor(TFT_WHITE, COLOR_CARD_BG);
  tft.setTextSize(3);
  tft.setCursor(50, 235);
  tft.print(buff);

  tft.setTextColor(COLOR_GAS, COLOR_CARD_BG);
  tft.setTextSize(1);
  tft.setCursor(85, 275);
  tft.print("RAW / PPM VALUE");
}

// Callback เมื่อรับข้อมูล ESP-NOW สำเร็จ
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t * info, const uint8_t *incomingData, int len) {
#else
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
#endif
    if (len == sizeof(sensorData)) {
      memcpy(&sensorData, incomingData, sizeof(sensorData));
      Serial.printf("Received -> Temp: %.1f C | TDS: %.0f PPM\n", sensorData.temp, sensorData.tds);
      updateDisplay();
    }
}

void setup() {
  Serial.begin(115200);

  // เปิดไฟ Backlight หน้าจอ CYD
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);

  tft.init();
  tft.setRotation(0); 
  
  sensorData.temp = 0.0;
  sensorData.ph = 0.0;
  sensorData.tds = 0.0;
  sensorData.turbidity = 0.0;
  sensorData.gas = 0;

  drawModernUI();
  updateDisplay();

  // ตั้งค่า WiFi และล็อคให้อยู่ Channel 1
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
}