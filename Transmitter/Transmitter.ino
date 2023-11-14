#include <SoftwareSerial.h>
#include "EBYTE.h"

#define PIN_RX 2
#define PIN_TX 3
#define PIN_M0 4
#define PIN_M1 5
#define PIN_AX 6

// Voltage sensor readings from analog 0 pin
const int VOL_PIN = A0;
// Current sensor readings from analog 1 pin
const int CUR_PIN = A1;

int value, valueI;
float volt, current, power;

SoftwareSerial ESerial(PIN_RX, PIN_TX);
EBYTE Transceiver(&ESerial, PIN_M0, PIN_M1, PIN_AX);

struct DATA {
  unsigned long Count;
  float Volts;
  float Amps;
  float Pow;

};
DATA DaData;



void setup() {
  
  Serial.begin(9600);
  ESerial.begin(9600);
  
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
  Serial.println(Transceiver.GetAirDataRate());
  Serial.println(Transceiver.GetChannel());
  Transceiver.SetMode(MODE_NORMAL);
  Transceiver.Reset();
  ESerial.flush();

}

void loop() {


  DaData.Count++;
  value = analogRead(VOL_PIN);
  delay(10);
  valueI = analogRead(CUR_PIN);
  delay(10);

  DaData.Volts = value / (((36149000.0 / 324803080.0) + (977.0 / 26977.0))) * (5.0 / 1023.0);
  DaData.Amps = (valueI * (5.0 / 1023.0) - 2.5) * 20.0;

  if (DaData.Amps < 0) {
    DaData.Amps = 0;
  }

  if (DaData.Volts > 0) {
    DaData.Volts += 0.134705882;
  }

    
  DaData.Pow = DaData.Volts * DaData.Amps;
  
  Transceiver.SendStruct(&DaData, sizeof(DaData));
  Serial.print("Sending: "); Serial.println(DaData.Count);
  Serial.print("Count: "); Serial.println(DaData.Count);
  Serial.print("Amps: "); Serial.println(DaData.Amps);
  Serial.print("Volts: "); Serial.println(DaData.Volts);
  Serial.print("Power: "); Serial.println(DaData.Pow);

  smartdelay(2000);

  delay(5000);

}

void smartdelay(unsigned long timeout){
  unsigned long t = millis();
  while (digitalRead(PIN_AX)== LOW){
    if ((millis()-t)>timeout){
      break;
    }
  }
  t = millis();
  while ((millis()-t)<20){};
}
