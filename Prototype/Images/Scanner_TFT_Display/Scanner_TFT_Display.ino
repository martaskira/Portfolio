/*
Based on Neil Kolban example for IDF:
https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
Ported to Arduino ESP32 by Evandro Copercini
Changed to a beacon scanner to report iBeacon, EddystoneURL and EddystoneTLM beacons by beegee-tokyo
Upgraded Eddystone part by Tomas Pilny on Feb 20, 2023
ESP32-S3 TFT Display and detection warning for 'Cat_Beacon' added by Brian Nov 24, 2023
*/
// libraries
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEEddystoneURL.h>
#include <BLEEddystoneTLM.h>
#include <BLEBeacon.h>
#include <Adafruit_ST7789.h>
// library for the TFT display
#include <Fonts/FreeMono9pt7b.h>
// objects
Adafruit_ST7789 display = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST); // TFT object named display
GFXcanvas16 canvas(240, 135); // graphics buffer object named canvas
int scanTime = 5; // In seconds
BLEScan *pBLEScan;
// variables
String targetBeacon_str = "";
int8_t signalStrength_int = 0;
String output_str = "";
int ledPin = 13; // GPIO LED on the ESP32-S3 acts as a warning promixity LED

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveName()) {
      targetBeacon_str = advertisedDevice.getName().c_str();
      // signal strength is in decibels: > -50 is close; < -80 is far
      signalStrength_int = advertisedDevice.getRSSI();
      // rule out medium-distant beacons as irrelevant, to simplify reporting
      if(signalStrength_int > -70) {
        // output reading to serial monitor
        Serial.print(targetBeacon_str);
        Serial.print(",");
        Serial.println(signalStrength_int);
        // compose message to output to the TFT display
        output_str = targetBeacon_str;
        output_str += ", ";
        output_str += signalStrength_int;
        canvas.println(output_str);
        if(targetBeacon_str=="Cat_Beacon")
          if(signalStrength_int > -65)
            // LED lights if cat is detected (adjust value as necessary)
            digitalWrite(ledPin, HIGH);
          else
            digitalWrite(ledPin, LOW);
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(100);
  // turn on the TFT / I2C power supply
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);

  display.init(135, 240); // Init ST7789 240x135
  display.setRotation(3);
  canvas.setFont(&FreeMono9pt7b);
  canvas.setTextColor(ST77XX_WHITE);
  // set pinmode for LED (GPIO13 for the ESP32-S3 TFT, probably 2 for the NodeMCU)
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); // LED initially off
  Serial.println("Scanning...");
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value
}

void loop() {
  // clear screen
  canvas.fillScreen(ST77XX_BLACK);
  canvas.setCursor(0, 25);
  // put your main code here, to run repeatedly:
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  // output to serial monitor
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");
  // send graphics buffer to TFT display
  display.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 135);
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);
  pBLEScan->clearResults(); // delete results from BLEScan buffer to release memory
  delay(2000); // wait 2 seconds, hardware needs time
}
