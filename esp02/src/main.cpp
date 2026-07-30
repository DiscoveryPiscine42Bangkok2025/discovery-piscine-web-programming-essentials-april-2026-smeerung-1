#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <esp_wifi.h>

// ==================== ตั้งค่า Pin (กลุ่ม ADC1 ทั้งหมด) ====================
#define TEMP_PIN      27 // DS18B20 อุณหภูมิ
#define PH_PIN        32 // pH Sensor (ADC1_CH4)
#define GAS_PIN       33 // Gas Sensor (ADC1_CH8)
#define TURBIDITY_PIN 34 // Turbidity Sensor (ADC1_CH6)
#define TDS_PIN       35 // TDS Sensor (ADC1_CH7)

// ขาสำหรับควบคุมไฟ LED แสดงสถานะ
#define LED_GREEN_PIN 25 // LED สีเขียว (Pass 🟢)
#define LED_RED_PIN   26 // LED สีแดง (Fail 🔴)

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

// MAC Address ของตัวรับ CYD
uint8_t receiverAddress[] = {0x20, 0x50, 0x0D, 0x1A, 0x99, 0x30};

// โครงสร้างข้อมูล ESP-NOW (ตรงกับ CYD 100%)
typedef struct struct_message {
    float temp;
    float ph;
    float tds;
    float turbidity;
    int gas;
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

// 1. ฟังก์ชันอ่านค่า TDS (ชดเชยอุณหภูมิ)
float readTDS(float currentTemp) {
  int analogValue = analogRead(TDS_PIN);
  float voltage = analogValue * (3.3 / 4095.0);
  
  float compensationCoefficient = 1.0 + 0.02 * (currentTemp - 25.0);
  float compensationVoltage = voltage / compensationCoefficient;
  
  float tdsValue = (133.42 * pow(compensationVoltage, 3) 
                   - 255.86 * pow(compensationVoltage, 2) 
                   + 857.39 * compensationVoltage) * 0.5;
  
  if (tdsValue < 0) tdsValue = 0;
  return tdsValue;
}

// 2. ฟังก์ชันอ่านค่า pH
float readPH() {
  int analogValue = 0;
  for (int i = 0; i < 10; i++) {
    analogValue += analogRead(PH_PIN);
    delay(10);
  }
  analogValue /= 10;

  float voltage = analogValue * (3.3 / 4095.0);
  float phValue = 3.5 * voltage;
  
  if (phValue < 0.0) phValue = 0.0;
  if (phValue > 14.0) phValue = 14.0;
  return phValue;
}

// 3. ฟังก์ชันอ่านค่าความขุ่น Turbidity
float readTurbidity() {
  int analogValue = 0;
  for (int i = 0; i < 10; i++) {
    analogValue += analogRead(TURBIDITY_PIN);
    delay(10);
  }
  analogValue /= 10;

  float voltage = analogValue * (3.3 / 4095.0);
  float ntu = 0;
  
  if (voltage < 2.5) {
    ntu = 3000.0;
  } else {
    ntu = -1120.4 * pow(voltage, 2) + 5742.3 * voltage - 4353.8;
  }

  if (ntu < 0) ntu = 0;
  return ntu;
}

// 4. ฟังก์ชันอ่านค่า Gas
int readGas() {
  long rawValue = 0;
  for (int i = 0; i < 10; i++) {
    rawValue += analogRead(GAS_PIN);
    delay(10);
  }
  rawValue /= 10;
  return (int)rawValue;
}

// 5. ฟังก์ชันตรวจสอบสภาพน้ำ (อิงตามตาราง Pass / Fail) *ไม่รวมก๊าซ*
bool isWaterClean(float temp, float ph, float tds, float turbidity) {
  bool tempOk = (temp >= 15.0 && temp <= 38.0); // อุณหภูมิ 15.0C - 38.0C
  bool phOk   = (ph >= 6.5 && ph <= 8.5);       // pH 6.5 - 8.5
  bool tdsOk  = (tds <= 300.0);                 // TDS <= 300 ppm
  bool turbOk = (turbidity <= 5.0);             // Turbidity <= 5 NTU

  // ต้องผ่านเกณฑ์น้ำครบทุกข้อ จึงจะถือว่า "ผ่าน (Pass)"
  return (tempOk && phOk && tdsOk && turbOk);
}

// Callback ตรวจสอบการส่งข้อมูล
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#else
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#endif
  Serial.print("Delivery Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success 🟢" : "Fail 🔴");
}

void setup() {
  Serial.begin(115200);

  // ตั้งค่า Pin เซ็นเซอร์
  pinMode(TDS_PIN, INPUT);
  pinMode(PH_PIN, INPUT);
  pinMode(TURBIDITY_PIN, INPUT);
  pinMode(GAS_PIN, INPUT);
  
  // ตั้งค่า Pin LED
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);

  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_RED_PIN, LOW);

  sensors.begin();

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 1; 
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  // 1. อ่านค่าอุณหภูมิ (DS18B20)
  sensors.requestTemperatures(); 
  float currentTemp = sensors.getTempCByIndex(0);
  myData.temp = (currentTemp != DEVICE_DISCONNECTED_C) ? currentTemp : 25.0;

  // 2. อ่านค่าเซ็นเซอร์ต่างๆ
  myData.tds = readTDS(myData.temp);
  myData.ph = readPH();
  myData.turbidity = readTurbidity();
  myData.gas = readGas();

  // 3. ตรวจสอบสถานะน้ำตามตารางเกณฑ์ (ตัดแก๊สออก)
  bool pass = isWaterClean(myData.temp, myData.ph, myData.tds, myData.turbidity);

  if (pass) {
    digitalWrite(LED_GREEN_PIN, HIGH); // ไฟเขียวติด (PASS)
    digitalWrite(LED_RED_PIN, LOW);    // ไฟแดงดับ
    Serial.println("WATER STATUS: PASS (น้ำผ่านเกณฑ์) 🟢");
  } else {
    digitalWrite(LED_GREEN_PIN, LOW);  // ไฟเขียวดับ
    digitalWrite(LED_RED_PIN, HIGH);   // ไฟแดงติด (FAIL)
    Serial.println("WATER STATUS: FAIL (มีค่าตกเกณฑ์) 🔴");
  }

  // 4. แสดงค่าผ่าน Serial Monitor
  Serial.printf("Temp: %.1f C | pH: %.2f | TDS: %.0f PPM | Turb: %.2f NTU | Gas: %d\n", 
                myData.temp, myData.ph, myData.tds, myData.turbidity, myData.gas);

  // 5. ส่งข้อมูลไปยัง CYD
  esp_now_send(receiverAddress, (uint8_t *) &myData, sizeof(myData));

  delay(2000); 
}