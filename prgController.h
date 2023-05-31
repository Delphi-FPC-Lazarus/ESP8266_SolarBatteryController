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
    
    bool triggerStatCharge();
    bool triggerStopCharge();

    bool triggerStatChargeEmergency();
    bool triggerStopChargeEmergency();

    bool triggerStatDischarge();
    bool triggerStopDischarge();
  public:
    String GetStateString();
    void SetStandbyMode();

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void Init();
    void Handle();
};
Prg_Controller prg_Controller;

/*
const float pvDayNight=5;               // Tag Nacht Erkennung über die PV Anlage
*/

const float emeterChargePower=-500;         // Trigger das Laden begonnen werden kann (entspricht mind. Ladeleistung des Batterieladers) (negativ weil Trigger auf Einspeisung)
const float emeterDischargePower=100;       // Trigger das Entladen begonnen werden kann (entspricht mind. Entladeleistung des Wandlers) (positiv weil Trigger auf Bezug)
const float emeterDischargeStopPower=-25;   // Trigger das Entladen abzubrechen (im normalfall 0 weil ich nicht aus dem Akku einspeisen will, etwas tolleranz gewähren) (negativ weil Trigger auf Einspeisung)

const float battEmergencyStart=10;          // %Akku Ladug bei der die Ladung unabhängig von Solarleistung gestartet wird um Schaden am Akku zu verhindern
const float battEmergencyStop=70;           // %Akku Ladug bei der der Akkuschutz aufhört zu laden
const float battFull=95;                    // %Akku bei der der Akku als voll betrachtet wird, also keine Ladung mehr gestartet wird
const float battApplicable=50;              // %Akku die mindestens vorhanden sein muss um den Einspeisevorgang zu starten
const float battStopDischarge=20;           // Entladevoragnag stoppen (zu prüfen ob die Spannungsmessung während dem Enladen richtig fuktioniert)

// --------------------------------------------
// Sicherheitsabschaltung

bool Prg_Controller::CheckFailure() {
  // Abschaltung weil Akkufehler, BMS hat abgeschaltet, Sicherung geflogen, hier können später noch weiter Bedingungen aufgenommen werden.
  delay(1); // Yield()
  mod_IO.MeasureBattGes(false);
  delay(1); // Yield()
  if (mod_IO.vBatt_gesProz < 1) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGes, mod_IO.vBatt_ges);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGesProz, mod_IO.vBatt_gesProz);
    return true;
  } else {
    return false;
  }
}

// --------------------------------------------
// Hilfsfunktionen 

boolean Prg_Controller::isDay() {
  // Result True = Tag , Result False = Nacht
  if ( 
        (mod_Timer.runTime.h >= 9) && (mod_Timer.runTime.h <= 18) 
     ) {
     return true;
  } else {
    return false;
  } 
}

/*
boolean Prg_Controller::isDay() {
  // Ist es Tag? Unabhängig von der Uhrzeit einfach über die PV Anlage, die liefert am Tag immer über 0 (sogar bei Schnee)
  // Result True = Tag , Result False = Nacht
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
*/

// --------------------------------------------
// Ladetrigger
// Es muss unbedingt ein Autmatik Lader sowie ein Akku mit BMS verwendet werden der den Ladevorgang für den Akku automatisch regelt und bei der entsprechenden Ladeschlussspannung abschaltet

bool Prg_Controller::triggerStatCharge() {
  // Starttrigger für das reguläre Laden des Akkus
  // Wenn die Batterie nicht voll ist und genügend Überschusseinspeisung zur Verfügung steht (in dem Zustand wird nicht geladen, also Einspeisung/Überschuss muss mindestens Ladeleistung sein)
  // ggf. noch Zeitbegrenzung das er nicht zu früh anfängt wegen Geräuschentwicklung, technisch ist das nicht nötig

  delay(1); // Yield()
  mod_IO.MeasureBattGes(false);
  delay(1); // Yield()
  float emeterPower = mod_EMeterClient.GetCurrentPower(false);  // < 0 Einspeisung | > 0 Bezug
  if ( emeterPower == 0 ) { return false; } // Fehler wenn genau 0
  delay(1); // Yield()

  if (  
        (mod_IO.vBatt_gesProz <= battFull) && 
        (emeterPower < emeterChargePower)  &&
        (isDay() == true) 
     ) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGes, mod_IO.vBatt_ges);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGesProz, mod_IO.vBatt_gesProz);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_EMeterPower, emeterPower);
    return true;
  }
  else {
    return false;
  }
}

bool Prg_Controller::triggerStopCharge() {
  // Stoptrigger für das reguläre Laden des Akkus
  // Wenn in den Bezug gegangen wird (in dem zustand wird geladen, also sobald Ladeleistung+Sonstiger Verbrauch > Produktion)
  // ggf. noch eine Zeitbegrenzung, technisch aber nicht nötig
  // Hier nicht auf auf Spannung triggern (ggf. wird hier später noch eine Erkennung eingebaut wenn das Ladegerät abgeschaltet hat)

  delay(1); // Yield()
  float emeterPower = mod_EMeterClient.GetCurrentPower(false);  // < 0 Einspeisung | > 0 Bezug
  if ( emeterPower == 0 ) { return false; } // Fehler wenn genau 0
  delay(1); // Yield()

  if ( 
        (emeterPower > 0) || 
        (isDay() == false)
     ) { 
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_EMeterPower, emeterPower);
    return true;
  }
  else {
    return false;
  }
}

bool Prg_Controller::triggerStatChargeEmergency() {
  // Starttrigger für Notfall Laden wenn der Akku zu weit runter ist, egal ob die Sonne scheint oder nicht
  // Wenn die Batteriespannung zu tief abgesackt ist

  delay(1); // Yield()
  mod_IO.MeasureBattGes(false);
  delay(1); // Yield()

  if ( 
        (mod_IO.vBatt_gesProz <= battEmergencyStart) && 
        (isDay() == true) && (mod_Timer.runTime.h > 12)
     ) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGes, mod_IO.vBatt_ges);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGesProz, mod_IO.vBatt_gesProz);
    return true;
  }
  else {
    return false;
  }
}

bool Prg_Controller::triggerStopChargeEmergency() {
  // Stoptrigger für das Notfall Laden des Akkus, egal ob die Sonne scheint oder nicht
  
  if ( 
        (isDay() == false) 
     ) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGes, mod_IO.vBatt_ges);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGesProz, mod_IO.vBatt_gesProz);
    return true;
  }
  else {
    return false;
  }
}

// --------------------------------------------
// Entladetrigger
// Es muss unbedingt ein Akku mit BMS verwendet werden, welches den Entladevorgang überwacht und abschaltet bevor die Spannung zu weit sinkt

bool Prg_Controller::triggerStatDischarge() {
  // Starttrigger für das Entladen
  // Wenn die Batterie genügend Ladung hat und der Bezug/Verbrauch (einspeisung grad nicht aktiv) > der zu erwartenden Entladeleisung liegt
  // Zusätzlich kann über die Zeit geprüft werden, dass das erst Abends passiert damit der Akku nicht bereits am Tag entladen wird  (je nach dem ob man das will oder nicht, bei kleinem Akku nicht), Achtung: Start/Stop Bedingung gemeinsam anpassen

  delay(1); // Yield()
  mod_IO.MeasureBattGes(false);
  delay(1); // Yield()
  float emeterPower = mod_EMeterClient.GetCurrentPower(false);  // < 0 Einspeisung | > 0 Bezug
  if ( emeterPower == 0 ) { return false; } // Fehler wenn genau 0
  delay(1); // Yield()

  if ( 
        (mod_IO.vBatt_gesProz >= battApplicable) && 
        (emeterPower > emeterDischargePower)
     ) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGes, mod_IO.vBatt_ges);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGesProz, mod_IO.vBatt_gesProz);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_EMeterPower, emeterPower);
    return true;
  }
  else {
    return false;
  }
}

bool Prg_Controller::triggerStopDischarge() {
  // Stoptrigger für das Entladen
  // gestoppt wenn entweder die EntladeSpannung unter das eingestellte limit geht (höher als vom BMS um den Akku zu schonen) oder der Energiebedarf in die Lieferung geht (einspeisung grad aktiv)
  // Zusätzlich kann die Zeit geprüft werden, also wenn die Nacht zu Ende ist. Achtung: Start/Stop Bedingung gemeinsam anpassen
  
  delay(1); // Yield()
  mod_IO.MeasureBattGes(false);
  delay(1); // Yield()
  if ( 
        (mod_IO.vBatt_gesProz <= battStopDischarge)
    ) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGes, mod_IO.vBatt_ges);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGesProz, mod_IO.vBatt_gesProz);
    return true;
  }

  delay(1); // Yield()
  float emeterPower = mod_EMeterClient.GetCurrentPower(false);  // < 0 Einspeisung | > 0 Bezug
  if ( emeterPower == 0 ) { return false; } // Fehler wenn genau 0
  delay(1); // Yield()
  if ( 
        (emeterPower < emeterDischargeStopPower)
    ) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_EMeterPower, emeterPower);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGes, mod_IO.vBatt_ges);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattGesProz, mod_IO.vBatt_gesProz);
    return true;
  }

  return false;
}

// --------------------------------------------
// Servicefunktionen

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

void Prg_Controller::SetStandbyMode() {
  state = State_Standby;
}

// --------------------------------------------
// Standardfunktionen

void Prg_Controller::Init() {
  Serial.println("prgController_Init()");
  
  // Initialzustand
  triggertime_bak = mod_Timer.runTime.m;
  state = State_Standby;

  mod_IO.Off();
  mod_IO.MeasureBattGes(true);

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

          mod_IO.Charge();

          state = State_Charge;
        }
        if (triggerStatChargeEmergency()) {
          Serial.println("triggerStatChargeEmergency");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StartChargeEmergency,0);

          mod_IO.Charge();
          state = State_ChargeEmergency;
        }
        if (triggerStatDischarge()) {
          Serial.println("triggerStatDischarge");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StartDischarge,0);

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

          state = State_Standby;
        }
        break;

      case State_ChargeEmergency:
        Serial.println("State_ChargeEmergency");
        if (triggerStopChargeEmergency()) {
          Serial.println("triggerStopChargeEmergency");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StopChargeEmergency,0);

          mod_IO.Off();
          state = State_Standby;
        }
        break;

      case State_Discharge:
        Serial.println("State_Discharge");
        if (triggerStopDischarge()) {
          Serial.println("triggerStopDischarge");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StopDischarge,0);

          mod_IO.Off();
          state = State_Standby;
        }
        break;

    }

  }

}

// --------------------------------------------