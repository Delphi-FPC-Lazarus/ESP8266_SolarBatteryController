// IO Modul

#pragma once

#include "modLogger.h"
#include "modTimer.h"

// --------------------------------------------------
class Mod_IO {
  private:
    bool manMode;
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
// Zur Pinbelegung Berücksichtigen:
// es gibt einen ADC 0
// die LED_BUILTIN ist auf DIO 2 (Boardabhängig!) 

const int ain_VBatt = 0;        // ADC VBatt
const int dout_VBattmode = 6;   // ADC für VBatt Messung umschalten 

const int dout_ACea = 7;        // AC ein/aus
const int dout_ACmode = 8;      // AC Modus laden/entladen (UM damit niemals gleichzeit)

// --------------------------------------------

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

  // 
}

void Mod_IO::Charge() {
  Serial.println("IO Charge");
  mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_IOcharge,0);

  // 
}

void Mod_IO::Discharge() {
  Serial.println("IO Discharge");
  mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_IOdischarge,0);

  //
}

void Mod_IO::MeasureBattGes(bool dolog) {
  Serial.println("IO MeasureBattGes");
  // MeasureBattGes und MeasureBatt12 müssen gegeneinander gegeneinander verriegelt werden
  // MeasureBattGes darf immer aufgerufen werden
  if (dolog == true) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Measure,0);
  }

  //

  if (dolog == true) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGes,0);
  }  
}

void Mod_IO::MeasureBatt12(bool dolog) {
  Serial.println("IO MeasureBatt12");
  // MeasureBattGes und MeasureBatt12 müssen gegeneinander gegeneinander verriegelt werden
  // MeasureBatt12 darf nicht immer aufgerufen werden da es Relaissteurung benötigt
  if (dolog == true) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Measure,0);
  }  

  // 

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

  /*
  // Digitale Ausgänge initialisieren
  pinMode(dout_VBattmode, OUTPUT); 
  digitalWrite(dout_VBattmode, LOW);
  
  pinMode(dout_ACea, OUTPUT); 
  digitalWrite(dout_ACea, LOW);

  pinMode(dout_ACmode, OUTPUT); 
  digitalWrite(dout_ACmode, LOW);
  */  

}

void Mod_IO::Handle() {
  // nur die BatterieMessung (ohne Relaissteurung) durchführen
  MeasureBattGes(false);
}

// --------------------------------------------
