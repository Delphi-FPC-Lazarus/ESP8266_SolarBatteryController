// IO Modul

#pragma once

#include "modLogger.h"
#include "modTimer.h"

// --------------------------------------------------
class Mod_IO {
  private:
    bool manIOMode;
    float manBattSimu;

    float VBattMeasurement();
    float vBattToProz(float spgvalue);
  public:
    void SetmanIOModeOn();
    void SetmanIOModeOff();
    bool IsmanIOMode();

    void SetmanBattSimuOn(float value);
    void SetmanBattSimuOff();
    
    void Off();
    void Charge();
    void Discharge();

    void MeasureBattGes(bool dolog);
    void MeasureBatt12(bool dolog);

    float vBatt_gesProz;
    float vBatt_ges;
    float vBatt_Batt1;
    float vBatt_Batt2;

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void Init();
    void Handle();
};
Mod_IO mod_IO;

// --------------------------------------------------

// Intern,
// Zur Pinbelegung Berücksichtigen (Boardabhängig!) Diese sollten hier nicht verwendet werden
const byte dout_LED = LED_BUILTIN;    //  D4 	GPIO 2 	2 ist die LED
const byte din_TASTER = D3;           //  D3 	GPIO 0 	0 ist der Taster
byte din_TasterLast = 0;
byte din_TasterNow = 0;

const int ain_VBatt = A0;        // ADC VBatt
const int dout_VBattmode = D5;   // ADC für VBatt Messung umschalten 

const int dout_ACea = D1;        // AC ein/aus
const int dout_ACmode = D2;      // AC Modus laden/entladen (UM damit niemals gleichzeit)

const float CALVOLT = 26.66;     // Kalibierung der Spannungsmessung über den analog in 
const int CALVALUE =  752;       // Kalibierung der Spannungsmessung über den analog in
const int CALOFFSET =  13;       // Kalibierung der Spannungsmessung über den analog in

const uint8_t R_ON = LOW;        // R_OFF aktiv
const uint8_t R_OFF = HIGH;      // Initialzustand R_ON 

// --------------------------------------------

float Mod_IO::vBattToProz(float spgvalue) {
  float proz = -1;

  // Tabelle nach Herstellerangabe für LiFePo4 Akku 
  // Achtung, diese Tabelle ist erst nach 30 Minuten Nullstrom gültig!
  // Während des Entladenes wird ein geringerer Wert angezeigt (erholt nach sich bei Nullstrom i.d.R. innerhalb von Minuten)

  if (spgvalue/2 >= 9.5)    { proz = 0; }
  if (spgvalue/2 >= 10.8)   { proz = 1; }
  if (spgvalue/2 >= 12.8)   { proz = 10; }
  if (spgvalue/2 >= 12.82)  { proz = 12; } // interpolierter Wert, ungenau
  if (spgvalue/2 >= 12.85)  { proz = 15; } // interpolierter Wert, ungenau
  if (spgvalue/2 >= 12.88)  { proz = 18; } // interpolierter Wert, ungenau
  if (spgvalue/2 >= 12.9)   { proz = 20; }
  if (spgvalue/2 >= 12.92)  { proz = 22; } // interpolierter Wert, ungenau
  if (spgvalue/2 >= 12.95)  { proz = 25; } // interpolierter Wert, ungenau
  if (spgvalue/2 >= 12.98)  { proz = 28; } // interpolierter Wert, ungenau
  if (spgvalue/2 >= 13.0)   { proz = 30; }
  if (spgvalue/2 >= 13.1)   { proz = 40; }
  if (spgvalue/2 >= 13.13)  { proz = 50; } // interpolierter Wert, ungenau
  if (spgvalue/2 >= 13.17)  { proz = 60; } // interpolierter Wert, ungenau
  if (spgvalue/2 >= 13.2)   { proz = 70; }
  if (spgvalue/2 >= 13.25)  { proz = 80; } // interpolierter Wert, ungenau
  if (spgvalue/2 >= 13.3)   { proz = 90; }
  if (spgvalue/2 >= 13.35)  { proz = 95; } // interpolierter Wert, ungenau
  if (spgvalue/2 >= 13.4)   { proz = 99; }
  if (spgvalue/2 >= 13.5)   { proz = 100; }

  return proz;
}

float Mod_IO::VBattMeasurement() {
  const int avgSkip = 500;
  const int avgCount = 1000;

  // adc springt nachdem sich die Versorgungspannung z.B. durch Relais minimal verschoben hat, erste Values wegwerfen
  for (int i = 0; i < avgSkip; i++) 
  {  
    int valuetrash = analogRead(ain_VBatt);
    //Serial.print("VBattMeasurement() Value(trash): "); Serial.println(valuetrash);
    //delay(1); // messen mit festenm intervall
    yield();    
    ESP.wdtFeed(); 
  }
  
  // Messen und Mitteln
  // Notiz: delay() < 1ms compiliert, tut aber nichts                                              
  // analogRead() ist träge, 0,1ms auf dem esp8266
  int value = 0;
  int valuesum = 0;
  for (int i = 0; i < avgCount; i++) 
  {  
    value = analogRead(ain_VBatt);
    //Serial.print("VBattMeasurement() Value: "); Serial.println(value);
    valuesum += value;
    //delay(1); // messen mit festenm intervall
    yield();    
    ESP.wdtFeed(); 
  }
  float valuekorr = ( valuesum / avgCount ) - CALOFFSET;
  float volt = CALVOLT/(float)CALVALUE * (float)valuekorr;
  return volt;
}

void Mod_IO::SetmanIOModeOn() {
  if (manIOMode != true) {
    manIOMode = true;
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_IOmanIOModeOn,0);
  }
}
void Mod_IO::SetmanIOModeOff() {
  if (manIOMode != false) {
    manIOMode = false;
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_IOmanIOModeOff,0);
  }
}
bool Mod_IO::IsmanIOMode() {
  return manIOMode;
}

void Mod_IO::SetmanBattSimuOn(float value) {
  manBattSimu = value;
  mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_IOmanBattSimuOn, value);
}

void Mod_IO::SetmanBattSimuOff() {
  if (manBattSimu > 0) {
    manBattSimu = -1;
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_IOmanBattSimuOff, 0);
  }
}

void Mod_IO::Off() {
  Serial.println("IO Off");
  mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_IOoff,0);

  // erst abschalten bevor lade/entlade Realais umgeschaltet werden
  digitalWrite(dout_ACea, R_OFF);
  delay(1000); // Hardware Zeit geben
  digitalWrite(dout_ACmode, R_OFF); 
}

void Mod_IO::Charge() {
  Serial.println("IO Charge");
  mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_IOcharge,0);

  // erst abschalten bevor lade/entlade Realais umgeschaltet werden
  digitalWrite(dout_ACea, R_OFF);
  delay(1000); // Hardware Zeit geben
  digitalWrite(dout_ACmode, R_OFF); 
  delay(1000); // Hardware Zeit geben
  digitalWrite(dout_ACea, R_ON);
}

void Mod_IO::Discharge() {
  Serial.println("IO Discharge");
  mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_IOdischarge,0);

  // erst abschalten bevor lade/entlade Realais umgeschaltet werden
  digitalWrite(dout_ACea, R_OFF);
  delay(1000); // Hardware Zeit geben
  digitalWrite(dout_ACmode, R_ON); 
  delay(1000); // Hardware Zeit geben
  digitalWrite(dout_ACea, R_ON);
}

void Mod_IO::MeasureBattGes(bool dolog) {
  Serial.println("IO MeasureBattGes");
  // MeasureBattGes und MeasureBatt12 müssten gegeneinander verriegelt werden, wir sind hier aber nicht multithreaded 
  // MeasureBattGes darf immer aufgerufen werden
  //if (dolog == true) {
  //  mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Measure,0);
  //}

  if (digitalRead(dout_VBattmode) == R_OFF) {
    if (manBattSimu > 0) {
      vBatt_ges = manBattSimu;
    } else {
      vBatt_ges = VBattMeasurement();
    }
    Serial.print("V Gemessen: "); Serial.println(vBatt_ges);
    vBatt_gesProz = vBattToProz(vBatt_ges);
    Serial.print("% Gemessen: "); Serial.println(vBatt_gesProz);
  }


  if (dolog == true) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGes, vBatt_ges);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGesProz, vBatt_gesProz);
  }  
}

void Mod_IO::MeasureBatt12(bool dolog) {
  Serial.println("IO MeasureBatt12");
  // MeasureBattGes und MeasureBatt12 müssten gegeneinander verriegelt werden, wir sind hier aber nicht multithreaded 
  // MeasureBatt12 darf nicht immer aufgerufen werden da es Relaissteurung benötigt
  //if (dolog == true) {
  //  mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Measure,0);
  //}  

  // Messe erstmal die gesamtspannung, dann Batt1, Batt2 ist dann die Differenz
  // Zu bachten ist, dass dies nur im Standby stimmt und während dem Lade-/Entlade-Vorgang 
  // aufgrund der indirekten Messmethode und Spannungsabfall an den Leitungen eine nicht vorhandene Abweichung zeigen kann
  Serial.println("Messe Gesamtspannung");
  if (manBattSimu > 0) {
    vBatt_ges = manBattSimu;
  } else {
    vBatt_ges = VBattMeasurement();
  }
  digitalWrite(dout_VBattmode, R_ON);
  delay(1000); // Hardware Zeit geben
  Serial.println("Messe Einzelspannung");
  if (manBattSimu > 0) {
    vBatt_Batt1 = manBattSimu / 2;
  } else {
    vBatt_Batt1 = VBattMeasurement();
  }
  digitalWrite(dout_VBattmode, R_OFF);
  delay(1000); // Hardware Zeit geben

  vBatt_Batt2 = vBatt_ges - vBatt_Batt1;

  Serial.println(vBatt_Batt1);
  Serial.println(vBatt_Batt2);

  if (dolog == true) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGes, vBatt_ges);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBatt1,vBatt_Batt1);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBatt2,vBatt_Batt2);
  }  
}


// --------------------------------------------
// Standard Init/Handler 

void Mod_IO::Init() {
  Serial.println("modIO_Init()");

  manIOMode = false;
  manBattSimu = -1;

  // Digitale Ausgänge initialisieren
  pinMode(dout_VBattmode, OUTPUT); 
  digitalWrite(dout_VBattmode, R_OFF);

  pinMode(dout_ACea, OUTPUT); 
  digitalWrite(dout_ACea, R_OFF);

  pinMode(dout_ACmode, OUTPUT); 
  digitalWrite(dout_ACmode, R_OFF);  

  // Digitale Eingänge initialisieren
  pinMode(0, INPUT_PULLUP);
  din_TasterLast = digitalRead(din_TASTER);  

  Serial.println("modIO_Init() Done");
}

void Mod_IO::Handle() {
  // hier nur Abfragen, keine Steuerung da zyklisch aufgerufen
  // z.B. Tasterabfrage und ggf. BatterieMessung (ohne Relaissteurung) durchführen
  // MeasureBattGes(false);

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
