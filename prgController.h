
#pragma once

// --------------------------------------------
class Prg_Controller {
  private:
    bool isDay();
    bool isNight(); 

    bool triggerStatCharge();
    bool triggerStatChargeEmergency();
    bool triggerStopCharge();

    bool triggerStatDischarge();
    bool triggerStopDischarge();
  public:
    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void Init();
    void Handle();
};
Prg_Controller prg_Controller;

const float pvDayNight=5;         // Tag Nacht Erkennung über die PV Anlage
const float pvLoadPower=750;      // Leistung der PV Anlage die zu Laden benötigt wird (Ladeleistung+HausStandby in erster Näheerung)     
const float battEmergency=25;     // %Akku Ladug bei der die Ladung unabhängig von Solarleistung gestartet wird um Schaden am Akku zu verhindern
const float battFull=95;          // %Akku bei der der Akku als voll betrachtet wird, also keine Ladung mehr gestartet wird
const float battApplicable=70;    // %Akku die mindestens vorhanden sein muss um den Einspeisevorgang zu starten
const float battStopDischarge=30; // Entladevoragnag stoppen

// --------------------------------------------

boolean Prg_Controller::isDay() {
  // Ist es Tag? Unabhängig von der Uhrzeit einfach über die PV Anlage, die liefert am Tag immer über 0 (sogar bei Schnee)
  float pvPower = mod_PVClient.GetCurrentPower(false);
  //Serial.println(pvPower); // Debug
  if (pvPower > pvDayNight) {
    return true;
  } else {
    return false;
  }
}

boolean Prg_Controller::isNight() {
  // Ist es Nacht? Unabhängig von der Uhrzeit einfach über die PV Anlage, die liefert nur in der Nacht wirklich 0
  float pvPower = mod_PVClient.GetCurrentPower(false);
  //Serial.println(pvPower); // Debug
  if (pvPower < pvDayNight) {
    return true;
  } else {
    return false;
  }  
}

bool Prg_Controller::triggerStatCharge() {
  // Starttrigger über PV Leistung (damit startet das nicht gleich früh sondern erst wenn genügend Sonne da ist)
  // und auch nur wenn die Batterie nicht voll ist
  float pvPower = mod_PVClient.GetCurrentPower(false);
  mod_IO.MeasureBattGes(false);
  if ( isDay() && (pvPower >= pvLoadPower) && (mod_IO.vBatt_gesProz <= battFull) ) {
    return true;
  }
  else {
    return false;
  }
}

bool Prg_Controller::triggerStatChargeEmergency() {
  // Starttrigger für Notfall Laden wenn der Akku zu weit runter ist, egal ob die Sonne scheint oder nicht
  // aber damit das nicht gleich ganz in der früh passiert, über Timer erst auf Mittag prüfen
  mod_IO.MeasureBattGes(false);
  if ( isDay() && (mod_IO.vBatt_gesProz <= battEmergency) && (mod_Timer.runTime.h >= 12) ) {
    return true;
  }
  else {
    return false;
  }
}

bool Prg_Controller::triggerStopCharge() {
  // Es muss unbedingt ein Autmatik Lader sowie ein Akku mit BMS verwendet werden der den Ladevorgang für den Akku automatisch regelt
  // und bei der entsprechenden Ladeschlussspannung abschaltet. Deshalb hier nicht auf auf Spannung triggern.
  // (ggf. wird hier später noch eine Erkennung eingebaut wenn das Ladegerät abgeschaltet hat)
  // Abgebrochen werden muss wenn die PV Leistung nicht mehr ausreichen würde den Akku mit Solarenergie zu laden
  float pvPower = mod_PVClient.GetCurrentPower(false);
  if ( isNight() || (pvPower < pvLoadPower) ) {
    return true;
  }
  else {
    return false;
  }
}

bool Prg_Controller::triggerStatDischarge() {
  // Entladen über die Nachterkennung und wenn der Akku entsprechend gefüllt ist
  mod_IO.MeasureBattGes(false);
  if ( isNight() && (mod_IO.vBatt_gesProz > battApplicable) ) {
    return true;
  }
  else {
    return false;
  }
}

bool Prg_Controller::triggerStopDischarge() {
  // Es muss unbedingt ein Akku mit BMS verwendet werden, welches den Entladevorgang überwacht
  // gestoppt wenn entweder die EntladeSpannung (höher als vom BMS um den Akku zu schonen) erreicht ist oder es wieder Tag ist
  mod_IO.MeasureBattGes(false);
  if ( isDay() || (mod_IO.vBatt_gesProz < battStopDischarge) ) {
    return true;
  }
  else {
    return false;
  }
}

// --------------------------------------------

const byte state_standby = 0;

byte triggertime_bak = 0;

void Prg_Controller::Init() {
  Serial.println("prgController_Init()");
  // Todo


}

void Prg_Controller::Handle() {
  // die Entscheidungsstruktur für den Modus triggert einmal pro Minute (nicht zu häufig)
  if ( (mod_Timer.runTime.m != triggertime_bak) && (mod_IO.IsmanIOMode() == false) ) {
    triggertime_bak = mod_Timer.runTime.m;
    //serial.println('Congroller Minutentrigger'); //  

    //Serial.print("isDay "); Serial.println(isDay());
    //Serial.print("isNight "); Serial.println(isNight());
  }

}

// --------------------------------------------