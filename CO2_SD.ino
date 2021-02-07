// This code is published under a Non-Commercial Attribution Creative Commons licence.
// References should mention:
// Pierre Carles, Polytech Sorbonne (January 2021)
// pierre.carles@sorbonne-universite.fr

#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);
RTC_PCF8523 rtc;
                                    // The pins associated with the three buttons on the
#define BUTTON_A  9                 // OLED Featherwing are specific to a given Feather board.
#define BUTTON_B  6                 // The present values are given for a M0 Proto/Express Feather.
#define BUTTON_C  5                 // Check documentation if you are using a different board.

#define Blue  13                    // Pins for the LED can be any digital I/O pins. However, several
#define Red  12                     // pins are already reserved for other functions: Pin 10 is
#define Green  11                   // assigned to communication with the micro-SD card, while pins 5,
                                    // 6 and 9 are assigned to the buttons on the OLED Featherwing.
float CO2_V = 0.; 
float logic_V = 3.3;                // Replace by 5 if you are using a 5V-logic board
                                    // (like and Arduino Uno).
unsigned int CO2_ppm;
unsigned int Threshold = 1000;      // Default CO2 threshold in ppm
unsigned int preheat = 120;         // Preheating time in seconds

const int chipSelect = 10;          // This is the SD Card CS pin number. 10 works for Feather M0, 328P
                                    // and Arduino Uno. It may differ for other boards.
unsigned long t,t0;
boolean Status_A = LOW;

String dataString = "";
String fileString = "LOG";
  
void setup() {

  SD.begin(chipSelect);
  analogReadResolution(12);         // Necessary if you want to use the full 12-bit range of the
                                    // Featherwing ADC. Should be commented for Uno Arduino boards.
  pinMode(Red, OUTPUT);
  pinMode(Green, OUTPUT);
  pinMode(Blue, OUTPUT);

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  delay(1000);                      // Gives the Featherwings time to boot.

  analogWrite(Red,0);
  analogWrite(Green,0);
  analogWrite(Blue,5);
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE,0x0000);
  display.setTextSize(2);
  display.println("Preheating");
  display.setTextSize(1);
  display.println();
  display.println("Please wait ...");
  display.display();

  t0 = millis();
  
  while (t < 1000*preheat) {            // This loop allows the user to skip the pre-heating phase
    if (digitalRead(BUTTON_A)==LOW) {   // (used for debugging purposes, not recommended otherwise).
      t = 1001*preheat;
      delay(200);
    }
    else {
      t = millis() - t0;
    }
  }
  
  analogWrite(Blue,0);

  display.clearDisplay();
  display.display();
}

void loop () {
  
  DateTime now = rtc.now();               // Gets the time from the RTC
  
  fileString += String(now.unixtime()).substring(4,9);
  fileString += ".txt";                   // These commands append a unique number to the name of the
                                          // log file every time the device is booted.
  while (HIGH) {                          // Once the filename is defined with its individual
                                          // time-stamp, an endless loops begins.
  DateTime now = rtc.now();
    
  display.setCursor(0,0);
  display.clearDisplay();
  display.display();

  CO2_V = logic_V*analogRead(A2)/4095.; // 4095 should be replaced by 1023 if using a board with
                                        // 10-bits ADCs like the Arduino Uno (remember then to also
  CO2_ppm = int((CO2_V - 0.4)*3125);    // comment the analogReadResolution command in the setup).

  display.setTextSize(1);
  
  if (CO2_V==0) {
    display.println("Error");
    display.println(" ");
  }
  else if (CO2_V < 0.4) {
    display.println("Preheating...");
    display.println(" ");
    
    display.setTextSize(2);
    display.print(CO2_V);
    display.println(" V"); 
  }
  else {
    displayhour(now.day());           // The displayhour function is defined below. It is used in order
    display.print('/');               // to add a '0' character to 1-digit times or dates, like in
    displayhour(now.month());         // 01/02/2021 (which, otherwise, woud be written 1/2/2021).
    display.print('/');
    display.print(now.year(), DEC);   // The date is written in European format. To display it in US
                                      // format, simply swap the day and month display commands.
    display.print(" ");
    
    displayhour(now.hour());          // Time is refreshed only once every 10 seconds, which may lead
    display.print(':');               // to minor discrepancies between the time displayed on the
    displayhour(now.minute());        // screen and actual time, on occasions. This does not affect
                                      // time as recorded on the log file.
    display.println();
    display.println(" ");

    display.setTextSize(2);
    display.print(CO2_ppm);
    display.println(" ppm");

    dataString = String(now.day());
    dataString += '/';
    dataString += String(now.month());
    dataString += '/';
    dataString += String(now.year());
    dataString += ',';
    dataString += String(now.hour());
    dataString += ':';
    dataString += String(now.minute());
    dataString += ':';
    dataString += String(now.second());
    dataString += ',';
    dataString += String(CO2_ppm);

    File dataFile = SD.open(fileString, FILE_WRITE);
    dataFile.println(dataString);     // Saves CO2 ppm value to the SD card.
    dataFile.close();
    }

  display.display();

  if (CO2_ppm <= Threshold) {
    analogWrite(Red,0);
    analogWrite(Green,3);
  }
  else {
    analogWrite(Green,0);
    analogWrite(Red,2);
  }
  
  t0 = millis();
  
  while (millis() - t0 < 9964) {      // This loop ensures a 10-second delay between two CO2
                                      // measurements (based on an estimated duration of 36 milliseconds
    if (digitalRead(BUTTON_A)==LOW) { // for all the preceding commands in the {while} loop).
      Status_A = HIGH;
      displaythreshold();
    }
    
    if (Status_A == HIGH) { 
      if (digitalRead(BUTTON_B)==LOW && Threshold < 5000) {
          Threshold = Threshold + 250;
          displaythreshold();
          delay(200);
        }
      else if (digitalRead(BUTTON_C)==LOW && Threshold > 0) {
          Threshold = Threshold - 250;
          displaythreshold();
          delay(200);
        }
    }
  }
  
  Status_A = LOW;
}
}

void displayhour(int number) {        // The displayhour funtion, allowing the display of a '0'
  if (number < 10) {                  // character in front of 1-digit day or month numbers.
    display.print('0');
    display.print(number);
  }
  else {
    display.print(number);
  }
}

void displaythreshold() {
  display.setCursor(0,0);
  display.clearDisplay();
  display.setTextSize(1);
  display.println("Choose Threshold");
  display.println(" ");
  display.setTextSize(2);
  display.print(Threshold);
  display.println(" ppm");
  display.display();
}
