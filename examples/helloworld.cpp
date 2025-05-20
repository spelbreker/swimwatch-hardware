//simple button masher esp32 ttgo
#include <TFT_eSPI.h>
#include <SPI.h>

#define BUTTON1PIN 14
#define BUTTON2PIN 0

TFT_eSPI tft = TFT_eSPI();

void IRAM_ATTR toggleButton1() {
  Serial.println("Button 1 Pressed!");
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 30);
  tft.drawLine(0, 35, 250, 35, TFT_BLUE);
  tft.setFreeFont(&Orbitron_Light_24);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.print("Button 1");
  tft.setCursor(0, 60);
  tft.print("Pressed!");
}

void IRAM_ATTR toggleButton2() {
  Serial.println("Button 2 Pressed!");
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 30);
  tft.drawLine(0, 35, 250, 35, TFT_BLUE);
  tft.setFreeFont(&Orbitron_Light_24);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.print("Button 2");
  tft.setCursor(0, 60);
  tft.print("Pressed!");
}

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON1PIN, INPUT);
  pinMode(BUTTON2PIN, INPUT);
  attachInterrupt(BUTTON1PIN, toggleButton1, FALLING);
  attachInterrupt(BUTTON2PIN, toggleButton2, FALLING);
  Serial.println("Test");

  tft.begin();
  tft.setRotation(3);
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  delay(3000);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(0, 30);
  tft.setFreeFont(&Orbitron_Light_24);
  tft.print("Press Button");
  tft.drawLine(0, 35, 250, 35, TFT_BLUE);
  tft.setCursor(0, 60);
  tft.print("3 Seconds Delay");
  Serial.println("serial line example");

}
