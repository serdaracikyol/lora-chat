#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>

#include "BluetoothSerial.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BLUETOOTH_DEVICE_NAME "Lora32 Sender"

// LoRa Module pin definitions.
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

// 433E6 for Asia
// 866E6 for Europe
// 915E6 for North America
#define BAND 866E6

// OLED pins
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST, 100000UL);

TaskHandle_t Task1;
TaskHandle_t Task2;
TaskHandle_t Task3;

String lastBlData;
String lastLoraData;

BluetoothSerial SerialBT;

void setup()
{
  Serial.begin(115200);

  // init Bluetooth Serial
  SerialBT.begin(BLUETOOTH_DEVICE_NAME);
    
  resetOLED();
  initOLED();
  drawSplashScreen();

  // SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  
  // setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(BAND))
  {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  
  xTaskCreatePinnedToCore(LoraSendTask,"LoraSendTask",10000,NULL,1,&Task1,0 /* Core Number*/);
  vTaskDelay(500);
  xTaskCreatePinnedToCore(LoraReceiveTask,"LoraReceiveTask",10000,NULL,1,&Task2,1 /* Core Number*/);
  vTaskDelay(500);
  xTaskCreatePinnedToCore(UpdateOLEDTask,"UpdateOLEDTask",10000,NULL,2,&Task3,1 /* Core Number*/);
  vTaskDelay(500);
}

void loop(){}

void LoraSendTask( void * pvParameters ){
  for(;;){
    // Send Message
    if (SerialBT.available()) {
      // Read data from Bluetooth
      lastBlData = SerialBT.readString();
      
      // Send LoRa packet to receiver
      LoRa.beginPacket();
      LoRa.print(lastBlData);
      LoRa.endPacket();
    }
    vTaskDelay(5);
  }
}

void LoraReceiveTask( void * pvParameters ){
  for(;;){
    int loraPacketSize = LoRa.parsePacket();
    
    if (loraPacketSize) {
      while (LoRa.available()) {
        // Read data from LoRa
        lastLoraData = LoRa.readString();

        // Send the data with Bluetooth
        SerialBT.print(lastLoraData);
      }
    }
    vTaskDelay(5);
  }  
}

void UpdateOLEDTask( void * pvParameters ){
    while(1){
      display.clearDisplay();
      display.setTextColor(WHITE);
      
      display.setCursor(30, 0);
      display.println("LORA CHAT");
      
      display.setCursor(0, 20);
      display.print("Gonderilen Mesaj:");
    
      display.setCursor(0, 30);
      display.print(lastBlData);
      
      display.setCursor(0, 40);
      display.print("Gelen Mesaj:");
    
      display.setCursor(0, 50);
      display.print(lastLoraData);
      
      display.display();

      vTaskDelay(500);
    }
}

void drawSplashScreen(){
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(30, 30);
  display.print("LORA CHAT");
  display.display();
}

void initOLED(){
  Wire.begin(OLED_SDA, OLED_SCL);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
}

void resetOLED(){
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH); 
}
