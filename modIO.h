// IO Modul

#pragma once

#include "modLogger.h"
#include "modTimer.h"

#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
//Adafruit_ADS1015 ads;     /* Use this for the 12-bit version */

// --------------------------------------------------
class Mod_IO {
  private:
    bool manIOMode;
    float manBatt1Simu;
    float manBatt2Simu;

    float vBattMeasurement(uint8_t channel);
    float vBattToProz(float spgvalue);
  public:
    void setManIOModeOn();
    void setManIOModeOff();
    bool isManIOMode();

    void setManBattSimuOn(byte battselect, float value);
    void setManBattSimuOff(byte battselect);
    
    void setOff();
    void setCharge();
    void setDischarge();

    bool isOff();

    // Batterie für den aktuellen Lade-/Entladevorgang
    byte getBattActive();
    void selectBattActive(byte battselect);
    void measureBattActive(bool dolog);
    float vBatt_active;
    float vBatt_activeproz;

    // Batterie 1/2 für den vorgeschalteten Auswahlprozess
    void measureBatt12(bool dolog);
    float vBatt_1;
    float vBatt_1proz;
    float vBatt_2;
    float vBatt_2proz;

    bool isBattActiveValid();
    bool isBatt1Valid();
    bool isBatt2Valid();

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void init();
    void handle();
};
Mod_IO mod_IO;

// --------------------------------------------------

// Intern,
// Zur Pinbelegung Berücksichtigen (Boardabhängig!) Diese sollten hier nicht verwendet werden
const byte scl = D1;                  //  D1  GPIO5  ist SCL des I2C
const byte sda = D2;                  //  D2  GPIO4  ist SDA des I2C
const byte dout_LED = LED_BUILTIN;    //  D4 	GPIO2  ist die LED
const byte din_TASTER = D3;           //  D3 	GPIO0  ist der Taster
byte din_TasterLast = 0;
byte din_TasterNow = 0;

const int dout_ACea = D5;        // AC aus/ein (off=aus on=ein)
const int dout_ACmode = D6;      // AC Modus laden/entladen, UM damit niemals gleichzeit (off=laden on=entladen)
const int dout_BATTselect = D7;  // DC Batterieumschalter

const uint8_t R_AC_OFF = HIGH;   // AC relaisboard inaktiv, Initialzustand 
const uint8_t R_AC_ON = LOW;     // AC relaisboard aktiv

const uint8_t R_BATT_OFF = LOW;   // AC relaisboard inaktiv, Initialzustand 
const uint8_t R_BATT_ON = HIGH;     // AC relaisboard aktiv

//const int ain_internal = A0;  // ADC intern hier nicht verwenden, von anderem Modul verwendet
const uint8 ain_batt1 = 0;      // ADC extern
const uint8 ain_batt2 = 1;      // ADC extern
bool extadcpresent = false;     // ADC extern vorhanden ja/nein

const float CALVOLT = 26.60;        // Kalibrierung der Spannungsmessung über den analog in (vor spannungsteiler)
const int CALVOLTVALUE =  18996;    // Kalibrierung der Spannungsmessung über den analog in (adc board nach spannungsteiler)
const int CALVOLTOFFSET =  0;       // Kalibrierung der Spannungsmessung über den analog in

// --------------------------------------------

float Mod_IO::vBattToProz(float spgvalue) {
  float proz = 0;

  // Tabelle nach Herstellerangabe für LiFePo4 Akku  zzgl. interpolierter Werte 
  // Achtung, diese Tabelle ist lt. Hersteller erst nach 30 Minuten Nullstrom gültig.
  // Während des Entladenes wird ein geringerer Wert angezeigt (erholt nach sich bei Nullstrom i.d.R. innerhalb sprunghaft innerhalb von Minuten)

  if (spgvalue/2 >= 9.5)    { proz = 0; }     // Herstellertabelle
  if (spgvalue/2 >= 10.8)   { proz = 1; }     // Herstellertabelle (Empfehlung der nierig Spannung Abschaltspannung) 
  if (spgvalue/2 >= 11.1)   { proz = 3; }     
  if (spgvalue/2 >= 11.8)   { proz = 5; }     
  if (spgvalue/2 >= 12.1)   { proz = 8; }     
  if (spgvalue/2 >= 12.8)   { proz = 10; }    // Herstellertabelle
  if (spgvalue/2 >= 12.82)  { proz = 12; }    
  if (spgvalue/2 >= 12.85)  { proz = 15; }    
  if (spgvalue/2 >= 12.88)  { proz = 18; }    
  if (spgvalue/2 >= 12.9)   { proz = 20; }    // Herstellertabelle
  if (spgvalue/2 >= 12.92)  { proz = 22; }    
  if (spgvalue/2 >= 12.95)  { proz = 25; }    
  if (spgvalue/2 >= 12.98)  { proz = 28; }    
  if (spgvalue/2 >= 13.0)   { proz = 30; }    // Herstellertabelle
  if (spgvalue/2 >= 13.05)  { proz = 35; }    
  if (spgvalue/2 >= 13.1)   { proz = 40; }    // Herstellertabelle
  if (spgvalue/2 >= 13.13)  { proz = 50; }    
  if (spgvalue/2 >= 13.17)  { proz = 60; }    
  if (spgvalue/2 >= 13.2)   { proz = 70; }    // Herstellertabelle
  if (spgvalue/2 >= 13.25)  { proz = 80; }    
  if (spgvalue/2 >= 13.3)   { proz = 90; }    // Herstellertabelle
  if (spgvalue/2 >= 13.35)  { proz = 95; }    
  if (spgvalue/2 >= 13.4)   { proz = 99; }    // Herstellertabelle 
  if (spgvalue/2 >= 13.5)   { proz = 100; }   // Herstellertabelle

  return proz;
}

float Mod_IO::vBattMeasurement(uint8_t channel) {
  if (!extadcpresent) {
    //mod_Logger.add(mod_Timer.runTimeAsString(),logCode_extADCMeasureFailed, channel);
    Serial.println("Messung nicht möglich!");
    return 0;
  }
  
  const int avgCount = 100;  // ca. 1 Sekunde wegen evtl. ripple auf der Spannung wenn Entladeung aktiv

  // Messen und Mitteln
  // Notiz: delay() < 1ms compiliert, tut aber nichts                                              
  // analogRead() ist träge, 0,1ms auf dem esp8266
  int value = 0;
  int valuesum = 0;
  for (int i = 0; i < avgCount; i++) 
  {  
    value = ads.readADC_SingleEnded(channel);
    //Serial.print("vBattMeasurement() Value: "); Serial.println(value);
    valuesum += value;
    //delay(1); // messen mit festenm intervall
    yield();    
    ESP.wdtFeed(); 
  }
  float valuekorr = ( valuesum / avgCount ) - CALVOLTOFFSET;
  float volt = CALVOLT/(float)CALVOLTVALUE * (float)valuekorr;
  
  Serial.print("vBattMeasurement() Value: "); Serial.println(value);
  Serial.print("vBattMeasurement() volt: "); Serial.println(volt);
 
  return volt;
}

void Mod_IO::setManIOModeOn() {
  if (manIOMode != true) {
    manIOMode = true;
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_IOmanIOModeOn,0);
  }
}
void Mod_IO::setManIOModeOff() {
  if (manIOMode != false) {
    manIOMode = false;
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_IOmanIOModeOff,0);
  }
}
bool Mod_IO::isManIOMode() {
  return manIOMode;
}

void Mod_IO::setManBattSimuOn(byte battselect, float value) {
  if (battselect == 1) {
    manBatt1Simu = value;
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_IOmanBattSimuOn, value);
  }
  if (battselect == 2) {
    manBatt2Simu = value;
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_IOmanBattSimuOn, value);
  }
}

void Mod_IO::setManBattSimuOff(byte battselect) {
  if (battselect == 1) {
    if (manBatt1Simu > 0) {
      manBatt1Simu = -1;
      mod_Logger.add(mod_Timer.runTimeAsString(),logCode_IOmanBattSimuOff, 1);
    }
  }
  if (battselect == 2) {
    if (manBatt2Simu > 0) {
      manBatt2Simu = -1;
      mod_Logger.add(mod_Timer.runTimeAsString(),logCode_IOmanBattSimuOff, 2);
    }
  }
}

void Mod_IO::setOff() {
  Serial.println("IO Off");
  mod_Logger.add(mod_Timer.runTimeAsString(),logCode_IOoff,0);

  // erst abschalten bevor lade/entlade Realais umgeschaltet werden
  digitalWrite(dout_ACea, R_AC_OFF);
  delay(1000); // Hardware Zeit geben
  digitalWrite(dout_ACmode, R_AC_OFF); 
}

void Mod_IO::setCharge() {
  Serial.println("IO Charge");
  mod_Logger.add(mod_Timer.runTimeAsString(),logCode_IOcharge,0);

  // erst abschalten bevor lade/entlade Realais umgeschaltet werden
  digitalWrite(dout_ACea, R_AC_OFF);
  delay(1000); // Hardware Zeit geben
  digitalWrite(dout_ACmode, R_AC_OFF); 
  delay(1000); // Hardware Zeit geben
  digitalWrite(dout_ACea, R_AC_ON);
}

void Mod_IO::setDischarge() {
  Serial.println("IO Discharge");
  mod_Logger.add(mod_Timer.runTimeAsString(),logCode_IOdischarge,0);

  // erst abschalten bevor lade/entlade Realais umgeschaltet werden
  digitalWrite(dout_ACea, R_AC_OFF);
  delay(1000); // Hardware Zeit geben
  digitalWrite(dout_ACmode, R_AC_ON); 
  delay(1000); // Hardware Zeit geben
  digitalWrite(dout_ACea, R_AC_ON);
}

bool Mod_IO::isOff() {
  return digitalRead(dout_ACea);
}

byte Mod_IO::getBattActive() {
  if (digitalRead(dout_BATTselect) == R_BATT_OFF) {
    return 1;
  } else { 
    return 2;
  }
}

void Mod_IO::selectBattActive(byte battselect) {
  Serial.print("IO battselect:"); Serial.println(battselect);
  mod_Logger.add(mod_Timer.runTimeAsString(),logCode_BattSelect,battselect);

  if (battselect == 1) {
    digitalWrite(dout_BATTselect, R_BATT_OFF);
  } else {
    digitalWrite(dout_BATTselect, R_BATT_ON);
  }
  delay(1000); // Hardware Zeit geben    

}

void Mod_IO::measureBattActive(bool dolog) {
  Serial.println("IO measureBattActive");

  // Messe die aktuell aktive Batterie (Batteriepack)
  // Zu bachten ist, dass dies nur im Standby stimmt und während dem Lade-/Entlade-Vorgang 
  // (diese Funktion wird ständig aufgerufen, daher hier keine Relais schalten)

  if (digitalRead(dout_BATTselect) == R_BATT_OFF) {
    if (manBatt1Simu > 0) {
      vBatt_active = manBatt1Simu;
    } else {
      vBatt_active = vBattMeasurement(ain_batt1);
    }
  } else {
    if (manBatt2Simu > 0) {
      vBatt_active = manBatt2Simu;
    } else {
      vBatt_active = vBattMeasurement(ain_batt2);
    }
  }
  
  Serial.print("V Gemessen Battaktiv: "); Serial.println(vBatt_active);
  vBatt_activeproz = vBattToProz(vBatt_active);
  Serial.print("% Gemessen Battaktiv:"); Serial.println(vBatt_activeproz);

  if (dolog == true) {    
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBattActive, vBatt_active);
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBattProz, vBatt_activeproz);
  }  
}

void Mod_IO::measureBatt12(bool dolog) {
  Serial.println("IO measureBatt12");

  // Messe nacheinander die Batterien (Batteriepacks)
  // Zu bachten ist, dass dies nur im Standby stimmt und während dem Lade-/Entlade-Vorgang 

  if (manBatt1Simu > 0) {
    vBatt_1 = manBatt1Simu;
  } else {
    vBatt_1 = vBattMeasurement(ain_batt1);
  }

  if (manBatt2Simu > 0) {
    vBatt_2 = manBatt2Simu;
  } else {
    vBatt_2 = vBattMeasurement(ain_batt2);
  }

  vBatt_1proz = vBattToProz(vBatt_1);
  vBatt_2proz = vBattToProz(vBatt_2);

  Serial.print("V Gemessen Batt1: "); Serial.println(vBatt_1);
  Serial.print("% Gemessen Batt1: "); Serial.println(vBatt_1proz);
  Serial.print("V Gemessen Batt2: "); Serial.println(vBatt_2);
  Serial.print("% Gemessen Batt2: "); Serial.println(vBatt_2proz);

  if (dolog == true) {
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBatt1,vBatt_1);
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBattProz, vBatt_1proz);
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBatt2,vBatt_2);
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBattProz, vBatt_2proz);
  }  
}

bool Mod_IO::isBattActiveValid() {
  return (mod_IO.vBatt_active > 1); // > 0  
}

bool Mod_IO::isBatt1Valid() {
  return (mod_IO.vBatt_1 > 1); // > 0 
}

bool Mod_IO::isBatt2Valid() {
  return (mod_IO.vBatt_2 > 1); // > 0 
}

// --------------------------------------------
// Standard Init/Handler 

void Mod_IO::init() {
  Serial.println("modIO_init()");

  manIOMode = false;
  manBatt1Simu = -1;
  manBatt2Simu = -1;

  vBatt_active = 0;
  vBatt_activeproz = 0;
  vBatt_1 = 0;
  vBatt_1proz = 0;
  vBatt_2 = 0;
  vBatt_2proz = 0;

  // Digitale Ausgänge initialisieren
  pinMode(dout_BATTselect, OUTPUT); 
  digitalWrite(dout_BATTselect, R_BATT_OFF); 

  pinMode(dout_ACea, OUTPUT); 
  digitalWrite(dout_ACea, R_AC_OFF);

  pinMode(dout_ACmode, OUTPUT); 
  digitalWrite(dout_ACmode, R_AC_OFF);  
 
  // Digitale Eingänge initialisieren
  pinMode(0, INPUT_PULLUP);
  din_TasterLast = digitalRead(din_TASTER);  

  // AD initialisieren https://github.com/adafruit/Adafruit_ADS1X15/
  
  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV

  if (ads.begin()) {
    Serial.println("ADS1X15 initialisierung ok");
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_extADCok, 0);
    extadcpresent = true;
  } else {
    Serial.println("ADS1X15 initialisierung fehlgeschalgen");
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_extADCfailed, 0);
    extadcpresent = false;
  }
  Serial.println("modIO_init() Done");
}

void Mod_IO::handle() {
  // hier nur Abfragen, keine Steuerung da zyklisch aufgerufen
  // z.B. Tasterabfrage und ggf. BatterieMessung (ohne Relaissteurung) durchführen
  // measureBattActive(false);

  din_TasterNow = digitalRead(din_TASTER);
  if (din_TasterNow != din_TasterLast) {
    Serial.print("Taster:"); Serial.println(din_TasterNow);    
    din_TasterLast = din_TasterNow;
    //if (din_TasterNow == true) {
    //} else {
    //}
  }

}

// --------------------------------------------
