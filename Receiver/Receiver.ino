
#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h> 
#include "RTClib.h"
#include "EBYTE.h"

#define PIN_RX 2 // Actually Tx pin
#define PIN_TX 3 // Actually Rx pin
#define PIN_M0 4
#define PIN_M1 5
#define PIN_AX 6

#define ECHO_TO_SERIAL 1
#define WAIT_TO_START 0

RTC_PCF8523 RTC;

struct DATA {
  unsigned long Count;
  float Volts;
  float Amps;
  float Pow;

};
DATA MyData;

const int chipSelect = 10;
unsigned long Last;

SoftwareSerial ESerial(PIN_RX, PIN_TX);
EBYTE Transceiver(&ESerial, PIN_M0, PIN_M1, PIN_AX);

File PowerData;

void setup() {
  Serial.begin(9600);
  ESerial.begin(9600);

  Serial.println("Initializing data storage...");
  pinMode(10, OUTPUT);

  if (!SD.begin(chipSelect)) {
    error("SD card not detected!");
  }

  Serial.println("Storage succesfully initialized!");

  char filename[] = "PW_LOG00.CSV";
  for(uint8_t i =0; i<100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      PowerData = SD.open(filename, FILE_WRITE);
      break;
    }
  }

  if (! PowerData) {
    error("File could not be created!");
  }

  Serial.print("Logging to: ");
  Serial.println(filename);

  Serial.println("Initializing RTC...");

  Wire.begin();
  if (! RTC.begin()) {
    PowerData.println("RTC error");
    #if ECHO_TO_SERIAL
      Serial.println("RTC error");
    #endif
  }
  else {
    Serial.println("RTC initialized");
  }

  
  //PowerData.println("Millis,Stamp,Year,Month,Day,Hour,Minute,Seconds,Volts,Amps,Pow");
  #if ECHO_TO_SERIAL
    Serial.println("Logging: Millis, Stamp, Datetime, Volts, Amps, Power");
  #endif
    
  Serial.println("Initializing transceiver...");
  Transceiver.init();
  Transceiver.SetMode(MODE_PROGRAM);
  Transceiver.SetUARTBaudRate(UDR_9600);
  Transceiver.SetTransmitPower(0b01);
  Transceiver.SetAirDataRate(0b010);
  Transceiver.SetAddressH(0);
  Transceiver.SetAddressL(0);
  Transceiver.SetChannel(13);
  Transceiver.SaveParameters(PERMANENT);
  Transceiver.Reset();

  Transceiver.PrintParameters();
  Transceiver.SetMode(MODE_NORMAL);
  Transceiver.Reset();
  ESerial.flush();
}

void loop() {
  DateTime now;

  uint32_t ms = millis();
  
  if (ESerial.available()) {
    Transceiver.GetStruct(&MyData, sizeof(MyData));
    Serial.print("Count: "); 
    Serial.println(MyData.Count);
    Serial.print("Amps: "); 
    Serial.println(MyData.Amps);
    Serial.print("Volts: "); 
    Serial.println(MyData.Volts);
    Serial.print("Power: "); 
    Serial.println(MyData.Pow);

    PowerData.print(ms);
    PowerData.print(", ");
    Serial.print("Timestamp: ");
    Serial.print(ms);
    Serial.print(", ");

    now = RTC.now();
    
    PowerData.print(now.unixtime()); // seconds since 1/1/1970
    PowerData.print(", ");
    //PowerData.print('"');
    PowerData.print(now.year(), DEC);
    //PowerData.print("/");
    PowerData.print(", ");
    PowerData.print(now.month(), DEC);
    //PowerData.print("/");
    PowerData.print(", ");
    PowerData.print(now.day(), DEC);
    PowerData.print(", ");
    PowerData.print(now.hour(), DEC);
    PowerData.print(", ");
    PowerData.print(now.minute(), DEC);
    PowerData.print(", ");
    PowerData.print(now.second(), DEC);
    //PowerData.println('"');

    #if ECHO_TO_SERIAL
    Serial.print(now.unixtime()); // seconds since 1/1/1970
    Serial.print(", ");
    Serial.print(now.year(), DEC);
    Serial.print("/");
    Serial.print(now.month(), DEC);
    Serial.print("/");
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    Serial.print(now.hour(), DEC);
    Serial.print(":");
    Serial.print(now.minute(), DEC);
    Serial.print(":");
    Serial.println(now.second(), DEC);
    #endif

    PowerData.print(", ");
    PowerData.print(MyData.Volts);
    PowerData.print(", ");
    PowerData.print(MyData.Amps);
    PowerData.print(", ");
    PowerData.print(MyData.Pow);
    PowerData.println();

    PowerData.flush();
    
    Last = millis();
  }
  else {
    if ((millis() - Last) > 1000) {
      Serial.println("Searching: ");
      Last = millis();
    }
  }
}

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);

  while(1);
}
