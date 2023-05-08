// Automatik Steuerung

#pragma once

enum PrgState {
  State_Failure,
  State_Standby,
  State_Charge, 
  State_ChargeEmergency,
  State_Discharge
};

// --------------------------------------------
class Prg_Controller {
  private:
    PrgState state;
    byte triggertime_bak;

    bool CheckFailure();

    bool isDay();
    bool isNight(); 

    bool triggerStatCharge();
    bool triggerStopCharge();

    bool triggerStatChargeEmergency();
    bool triggerStopChargeEmergency();

    bool triggerStatDischarge();
    bool triggerStopDischarge();
  public:
    String GetStateString();

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void Init();
    void Handle();
};
Prg_Controller prg_Controller;

const float pvDayNight=5;           // Tag Nacht Erkennung über die PV Anlage

const float emeterChargePower=-500; // Einspeisung (negativ) entsprechend Ladeleistung des Batterieladers

const float battEmergencyStart=25;  // %Akku Ladug bei der die Ladung unabhängig von Solarleistung gestartet wird um Schaden am Akku zu verhindern
const float battEmergencyStop=50;   // %Akku Ladug bei der der Akkuschutz aufhört zu laden (unterhalb der Mindestladung zur Verwendung aber genug um Akkuschäden vorzubeugen)
const float battFull=95;            // %Akku bei der der Akku als voll betrachtet wird, also keine Ladung mehr gestartet wird
const float battApplicable=70;      // %Akku die mindestens vorhanden sein muss um den Einspeisevorgang zu starten
const float battStopDischarge=30;   // Entladevoragnag stoppen

// --------------------------------------------

bool Prg_Controller::CheckFailure() {
  delay(1); // Yield()
  mod_IO.MeasureBattGes(false);
  delay(1); // Yield()
  // Sicherung geflogen oder Akkufehler, hier können später noch weiter Bedingungen aufgenommen werden
  if (mod_IO.vBatt_gesProz < 0) {
    return true;
  } else {
    return false;
  }
}

boolean Prg_Controller::isDay() {
  // Ist es Tag? Unabhängig von der Uhrzeit einfach über die PV Anlage, die liefert am Tag immer über 0 (sogar bei Schnee)
  delay(1); // Yield()
  float pvPower = mod_PVClient.GetCurrentPower(false);
  if ( pvPower < 0 ) { return false; } // Fehler
  delay(1); // Yield()
  //Serial.println(pvPower); // Debug
  if (pvPower > pvDayNight) {
    return true;
  } else {
    return false;
  }
}

boolean Prg_Controller::isNight() {
  // Ist es Nacht? Unabhängig von der Uhrzeit einfach über die PV Anlage, die liefert nur in der Nacht wirklich 0
  delay(1); // Yield()
  float pvPower = mod_PVClient.GetCurrentPower(false);
  if ( pvPower < 0 ) { return false; } // Fehler
  delay(1); // Yield()
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
  delay(1); // Yield()
  float emeterPower = mod_EMeterClient.GetCurrentPower(false);  // < 0 Einspeisung | > 0 Bezug
  if ( emeterPower == 0 ) { return false; } // Fehler wenn genau 0
  delay(1); // Yield()
  mod_IO.MeasureBattGes(false);
  delay(1); // Yield()
  if ( isDay() && (emeterPower < emeterChargePower) && (mod_IO.vBatt_gesProz <= battFull) ) {
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
  delay(1); // Yield()
  float emeterPower = mod_EMeterClient.GetCurrentPower(false);  // < 0 Einspeisung | > 0 Bezug
  if ( emeterPower == 0 ) { return false; } // Fehler wenn genau 0
  delay(1); // Yield()
  if ( isNight() || (emeterPower > 0) ) {
    return true;
  }
  else {
    return false;
  }
}

bool Prg_Controller::triggerStatChargeEmergency() {
  // Starttrigger für Notfall Laden wenn der Akku zu weit runter ist, egal ob die Sonne scheint oder nicht
  // aber damit das nicht gleich ganz in der früh passiert, über Timer erst auf Mittag prüfen
  delay(1); // Yield()
  mod_IO.MeasureBattGes(false);
  delay(1); // Yield()
  if ( isDay() && (mod_IO.vBatt_gesProz <= battEmergencyStart) && (mod_Timer.runTime.h >= 12) ) {
    return true;
  }
  else {
    return false;
  }
}

bool Prg_Controller::triggerStopChargeEmergency() {
  // Stoptrigger für das Notfall Laden, egal ob die Sonne scheint oder nicht
  delay(1); // Yield()
  mod_IO.MeasureBattGes(false);
  delay(1); // Yield()
  if ( isNight() || (mod_IO.vBatt_gesProz >= battEmergencyStop) ) {
    return true;
  }
  else {
    return false;
  }
}

bool Prg_Controller::triggerStatDischarge() {
  // Entladen über die Nachterkennung und wenn der Akku entsprechend gefüllt ist
  delay(1); // Yield()
  mod_IO.MeasureBattGes(false);
  delay(1); // Yield()
  if ( isNight() && (mod_IO.vBatt_gesProz >= battApplicable) ) {
    return true;
  }
  else {
    return false;
  }
}

bool Prg_Controller::triggerStopDischarge() {
  // Es muss unbedingt ein Akku mit BMS verwendet werden, welches den Entladevorgang überwacht
  // gestoppt wenn entweder die EntladeSpannung (höher als vom BMS um den Akku zu schonen) erreicht ist oder es wieder Tag ist
  delay(1); // Yield()
  mod_IO.MeasureBattGes(false);
  delay(1); // Yield()
  if ( isDay() || (mod_IO.vBatt_gesProz <= battStopDischarge) ) {
    return true;
  }
  else {
    return false;
  }
}

// --------------------------------------------
String Prg_Controller::GetStateString() {
  switch (state) {
      // Fehlerzustand
      case State_Failure:
        return "<font color=red>Systemfehler</font>";
        break;
      // Standby Zustand        
      case State_Standby:
        return "Standby";
        break;
      // Aktive Zustände
      case State_Charge:      
        return "<font color=green>Laden</font>";
        break;
      case State_ChargeEmergency:
        return "<font color=orange>Laden (Tiefentladeschutz)</font>";
        break;
      case State_Discharge:
        return "<font color=green>Entladen/Einspeisen</font>";
        break;
      default:
        return "<font color=red>Status unbekannt</font>";
        break;
  }
}

// --------------------------------------------

void Prg_Controller::Init() {
  Serial.println("prgController_Init()");
  
  // Initialzustand
  triggertime_bak = mod_Timer.runTime.m;
  state = State_Standby;

  mod_IO.Off();
  mod_IO.MeasureBattGes(true);
  mod_IO.MeasureBatt12(true);

  Serial.println("prgController_Init() Done");
}

void Prg_Controller::Handle() {
  // die Entscheidungsstruktur für den Modus triggert einmal pro Minute (nicht zu häufig)
  if ( (mod_Timer.runTime.m != triggertime_bak) && (mod_IO.IsmanIOMode() == false) ) {
    triggertime_bak = mod_Timer.runTime.m;
    Serial.println("Congroller Trigger");

    // Immer Fehlerprüfung aufrufen, in jedem Status außer wenn ich bereits im System failure status bin, dann ist eh alles tot
    if ( (state != State_Failure) && CheckFailure() ) {
      Serial.println("CheckFailure");
      mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_SystemFailure,0);

      mod_IO.Off();
      mod_IO.MeasureBattGes(true);
      mod_IO.MeasureBatt12(true);

      state = State_Failure;
    }

    switch (state) {
      // Fehlerzustand
      case State_Failure:
        // nichts mehr
        break;

      // Standby Zustand        
      case State_Standby:
        Serial.println("State_Standby");
        if (triggerStatCharge()) {
          Serial.println("triggerStatCharge");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StartCharge,0);

          mod_IO.MeasureBattGes(true);
          mod_IO.MeasureBatt12(true);
          mod_IO.Charge();

          state = State_Charge;
        }
        if (triggerStatChargeEmergency()) {
          Serial.println("triggerStatChargeEmergency");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StartChargeEmergency,0);

          mod_IO.MeasureBattGes(true);
          mod_IO.MeasureBatt12(true);
          mod_IO.Charge();
          state = State_ChargeEmergency;
        }
        if (triggerStatDischarge()) {
          Serial.println("triggerStatDischarge");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StartDischarge,0);

          mod_IO.MeasureBattGes(true);
          mod_IO.MeasureBatt12(true);
          mod_IO.Discharge();

          state = State_Discharge;
        }
        break;

      // Aktive Zustände
      case State_Charge:
        Serial.println("State_Charge");
        if (triggerStopCharge()) {
          Serial.println("triggerStopCharge");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StopCharge,0);

          mod_IO.Off();
          mod_IO.MeasureBattGes(true);
          mod_IO.MeasureBatt12(true);

          state = State_Standby;
        }
        break;

      case State_ChargeEmergency:
        Serial.println("State_ChargeEmergency");
        if (triggerStopChargeEmergency()) {
          Serial.println("triggerStopChargeEmergency");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StopChargeEmergency,0);
          mod_IO.MeasureBattGes(true);
          mod_IO.MeasureBatt12(true);

          mod_IO.Off();
          state = State_Standby;
        }
        break;

      case State_Discharge:
        Serial.println("State_Discharge");
        if (triggerStopDischarge()) {
          Serial.println("triggerStopDischarge");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StopDischarge,0);
          mod_IO.MeasureBattGes(true);
          mod_IO.MeasureBatt12(true);

          mod_IO.Off();
          state = State_Standby;
        }
        break;

    }

  }

}

// --------------------------------------------