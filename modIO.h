// IO Modul

#pragma once

#include "modLogger.h"
#include "modTimer.h"

// --------------------------------------------------
class Mod_IO {
  private:
    bool manMode;
    float VBattMeasurement();
  public:
    void SetManMode();
    bool IsManMode();
    
    void Off();
    void Charge();
    void Discharge();

    void MeasureBattGes(bool dolog);
    void MeasureBatt12(bool dolog);

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

const float CALVOLT = 30.0;      // Kalibierung der Spannungsmessung über den analog in 
const int CALVALUE = 1023;       // Kalibierung der Spannungsmessung über den analog in

// --------------------------------------------


float Mod_IO::VBattMeasurement() {
  int value = analogRead(ain_VBatt);
  float volt = CALVOLT/(float)CALVALUE * (float)value;
  //return value;
  
  return 0;
}

void Mod_IO::SetManMode() {
  if (manMode != true) {
    manMode = true;
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_IOManMode,0);
  }
}
bool Mod_IO::IsManMode() {
  return manMode;
}

void Mod_IO::Off() {
  Serial.println("IO Off");
  mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_IOoff,0);

  // erst abschalten bevor lade/entlade Realais umgeschaltet werden
  digitalWrite(dout_ACea, LOW);
  delay(1000);
  digitalWrite(dout_ACmode, LOW); 
}

void Mod_IO::Charge() {
  Serial.println("IO Charge");
  mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_IOcharge,0);

  // erst abschalten bevor lade/entlade Realais umgeschaltet werden
  digitalWrite(dout_ACea, LOW);
  delay(1000);
  digitalWrite(dout_ACmode, LOW); 
  delay(1000);
  digitalWrite(dout_ACea, HIGH);
}

void Mod_IO::Discharge() {
  Serial.println("IO Discharge");
  mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_IOdischarge,0);

  // erst abschalten bevor lade/entlade Realais umgeschaltet werden
  digitalWrite(dout_ACea, LOW);
  delay(1000);
  digitalWrite(dout_ACmode, HIGH); 
  delay(1000);
  digitalWrite(dout_ACea, HIGH);
}

void Mod_IO::MeasureBattGes(bool dolog) {
  Serial.println("IO MeasureBattGes");
  // MeasureBattGes und MeasureBatt12 müssten gegeneinander verriegelt werden, wir sind hier aber nicht multithreaded 
  // MeasureBattGes darf immer aufgerufen werden
  if (dolog == true) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Measure,0);
  }

  if (digitalRead(dout_VBattmode) == LOW) {
    vBatt_ges = VBattMeasurement();
  }

  if (dolog == true) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGes, vBatt_ges);
  }  
}

void Mod_IO::MeasureBatt12(bool dolog) {
  Serial.println("IO MeasureBatt12");
  // MeasureBattGes und MeasureBatt12 müssten gegeneinander verriegelt werden, wir sind hier aber nicht multithreaded 
  // MeasureBatt12 darf nicht immer aufgerufen werden da es Relaissteurung benötigt
  if (dolog == true) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Measure,0);
  }  

  // Messe erstmal die gesamtspannung, dann Batt1, Batt2 ist dann die Differenz
  digitalWrite(dout_VBattmode, LOW);
  delay(1000);

  vBatt_ges = VBattMeasurement();

  digitalWrite(dout_VBattmode, HIGH);
  delay(1000);

  vBatt_Batt1 = VBattMeasurement();
  vBatt_Batt2 = vBatt_ges - vBatt_Batt1;

  digitalWrite(dout_VBattmode, LOW);

  if (dolog == true) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBatt1,0);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBatt2,0);
  }  
}


// --------------------------------------------
// Standard Init/Handler 

void Mod_IO::Init() {
  Serial.println("modIO_Init()");

  manMode = false;

  // Digitale Ausgänge initialisieren
  pinMode(dout_VBattmode, OUTPUT); 
  digitalWrite(dout_VBattmode, LOW);

  pinMode(dout_ACea, OUTPUT); 
  digitalWrite(dout_ACea, LOW);

  pinMode(dout_ACmode, OUTPUT); 
  digitalWrite(dout_ACmode, LOW);  

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
