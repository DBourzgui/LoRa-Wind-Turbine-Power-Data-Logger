/*
 * Driss Bourzgui
 * UMass Lowell
 * Engineers for a Sustainable World (ESW)
 * 3/2023
 * 
 * Voltage, Current, and Power Sensor:
 * Description: Reads and calculates voltage and current values from analog pin 0  
 * (V) and analog pin 1 (I). Power is calculated from results and all values
 * are printed to the LCD, updating every second. A voltage error offset is 
 * used to achieve an average percent error of ~0.74% from 0V to 16V and ~0.15%
 * from 3V to 16V. Can read up to approximately 33.5V before over-voltage 
 * protection kicks in and up to +50A DC for current sensing (Max 6.72kW). 
 * Intended use is for measuring power delivered to a battery bank for a 12V 500W 
 * small scale wind turbine. Data is written to a CSV file and stored on an SD card. 
 * 
 * LCD Wiring:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 
 * Or
 * 
 * (Pin # Starting from right):
 * 14 - D2
 * 13 - D3
 * 12 - D4
 * 11 - D5
 * 10-7 - None
 * 6 - D11
 * 5 - GND
 * 4 - D12
 * 3- GND
 * 2 - 5V
 * 1 - GND

*/


#include <LiquidCrystal.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

#define LOG_INTERVAL  1000
#define ECHO_TO_SERIAL   1 
#define WAIT_TO_START    0
#define redLEDpin 2
#define greenLEDpin 3

uint32_t syncTime = 0;
RTC_PCF8523 RTC;
const int chipSelect = 10;

File logfile;

// Voltage sensor readings from analog 0 pin
const int VOL_PIN = A0;
// Current sensor readings from analog 1 pin
const int CUR_PIN = A1;

// Register select, enable, and data pins assigned to arduino data pins
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Arrays for storing custom graphics. Each is a single frame
// for a basic wind turbine animation. Each array value represents
// a row of pixels on the LCD.
byte frame1[8] = {
  0b00000,
  0b00100,
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b01110
};

byte frame2[8] = {
  0b00000,
  0b10000,
  0b01000,
  0b00111,
  0b01100,
  0b10100,
  0b00100,
  0b01110
};

byte frame3[8] = {
  0b00000,
  0b10001,
  0b01010,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b01110
};

byte frame4[8] = {
  0b00000,
  0b00001,
  0b00010,
  0b11100,
  0b00110,
  0b00101,
  0b00100,
  0b01110
};

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  
  // red LED indicates error
  digitalWrite(redLEDpin, HIGH);

  while(1);
}


void setup() {

  
  // Baud rate for serial monitor
  Serial.begin( 9600 );
  Serial.println();

  pinMode(redLEDpin, OUTPUT);
  pinMode(greenLEDpin, OUTPUT);

  #if WAIT_TO_START
    Serial.println("Type any character to start");
    while (!Serial.available());
  #endif

  Serial.print("Initializing SD card...");
  pinMode(10, OUTPUT);

  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");
  }
  Serial.println("card initialized.");

  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break; 
    }
  }

  if (! logfile) {
    error("couldnt create file");
  }

  Serial.print("Logging to: ");
  Serial.println(filename);

  Wire.begin();  
  if (!RTC.begin()) {
    logfile.println("RTC failed");
  #if ECHO_TO_SERIAL
    Serial.println("RTC failed");
  #endif  //ECHO_TO_SERIAL
  }

  logfile.println("millis,stamp,datetime,light,temp,vcc");    
  #if ECHO_TO_SERIAL
  Serial.println("millis,stamp,datetime,light,temp,vcc");
  #endif
  // Sets up the LCD's number of columns and rows
  lcd.begin(16, 2);
  
  // Prints data types to lcd
  lcd.print("V:");
  lcd.setCursor(9, 0);
  lcd.print("I:");
  lcd.setCursor(0, 1);
  lcd.print("P:");

  // Creates custom characters using defined arrays 
  lcd.createChar(0, frame1);
  lcd.createChar(1, frame2);
  lcd.createChar(2, frame3);
  lcd.createChar(3, frame4); 
 
}

void loop() {

  // Analog pin values and calculated voltage, current and power values
  int value, valueI;
  float volt, current, power;
  

  // Stores raw values read from analog pins to be used in calculations
  value = analogRead( VOL_PIN );
  valueI = analogRead( CUR_PIN );

  // Calculates original voltage from known resistor values, op-amp gain, and arduino resolution
  volt = value / (((36149000.0 / 324803080.0) + (977.0 / 26977.0))) * (5.0 / 1023.0);

  // Recalculates current from sensor range (+2.5V to +5.0V) and arduino resolution
  current = (valueI * (5.0 / 1023.0) - 2.5) * 20.0;

  // Current formula only valid for I > 0, and only measuring forward current (hopefully)
  if (current < 0) {
    current = 0;
  }

  //Calculates power in Watts
  power = volt * current;
  
  // Voltage error offset added to calculated value.
  // 0V is accurately read so offset not required
  if (volt > 0) {
    volt = volt + 0.134705882;
  }
  
  // Counting columns and rows starts from 0 from the left btw
  lcd.setCursor(2, 0);
  
  // Prints the measured/calculated voltage value to the lcd
  lcd.print(volt, 2);
  lcd.setCursor(7, 0);
  lcd.print("V");

  // Prints the measured/calculated current value to the lcd 
  lcd.setCursor(11, 0);
  lcd.print(current, 2);
  lcd.setCursor(15, 0);
  lcd.print("A");

  // Prints the calculated power value to the lcd
  lcd.setCursor(2, 1);
  lcd.print(power, 2);
  lcd.setCursor(7, 2);
  lcd.print("W");
  lcd.setCursor(8, 2);
  lcd.print(" ");

/* For Voltage Sensing only:
  if (volt <= 12.5 && volt >= 9.0) {
    lcd.setCursor(0, 1);
    lcd.print("Cooking w/ gas! ");
  }
  else if (volt > 12.5) {
    lcd.setCursor(0, 1);
    lcd.print("Battery full!");
  }
  else if (volt < 9.0 && volt > 2.0) {
    lcd.setCursor(0, 1);
    lcd.print("Nice & breezy...");
  }
  else {
    lcd.setCursor(0, 1);
    lcd.print("= Avg IQ of MEs!");
  }
*/

// Loop for a mini turbine animation on LCD 
// Each iteration prints one frame of animation
// to three locations. Delays stagger animations
// and controls animation speed. 
for (int i = 0; i < 4; i++) {
  lcd.setCursor(9, 1);
  lcd.write(byte(i));
  delay(100);
  
  lcd.setCursor(11, 1);
  lcd.write(byte(i));
  delay(100);

  lcd.setCursor(13, 1);
  lcd.write(byte(i));
  delay(100); 
}


  // Prints the current, voltage, and power values to serial monitor for testing purposes
  Serial.print( "  V: " );
  Serial.println( volt );
  Serial.print( "\n I: ");
  Serial.println( current );
  Serial.print( "\n P: ");
  Serial.println( power );
  Serial.println( valueI );
  //Serial.print( now.hour(), DEC );

 
}
