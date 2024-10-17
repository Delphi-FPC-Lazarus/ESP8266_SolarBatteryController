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
    
    int pwrControlSkip;
    float lastEMeterpwr;
    float lastWRpwrset;

    String detailsMsg;

    int chargeEndCounter;

    bool CheckFailure();

    bool isDay();
    
    bool triggerStatCharge();
    bool triggerStopCharge();

    bool triggerStatChargeEmergency();
    bool triggerStopChargeEmergency();

    bool triggerStartDischarge();
    bool triggerStopDischarge();

    void doPowerControl();

  public:
    String GetState();
    String GetStateString();
    String GetDetailsMsg();
    
    void SetStandbyMode();

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void Init();
    void Handle();
};
Prg_Controller prg_Controller;

/*
const float pvDayNight=5;               // Tag Nacht Erkennung über die PV Anlage
*/

// Ladeleistung 500W
const float emeterChargePower=-500;         // Trigger das Laden begonnen werden kann (entspricht mind. Ladeleistung des Batterieladers) (negativ weil Trigger auf Einspeisung)
const float chargeDetectPower=300;          // Erkennung des Lademodus bzw. Erkennung des Ladeenedes

// Entladeleistung 
const float defaultWRpwrset=150;            // Vorgabewert beim Einschalten (wandler fährt eh erst mal rampe, regelung erst nach max 10 min möglich)
const float maxWRpwrset=300;                // Maximalwert für den Wechselrichter
const float minWRpwrset=10;                 // Minimalwert für den Wechselrichter

const float emeterDischargePower=50;        // Trigger das Entladen begonnen werden kann (entspricht mind. Entladeleistung des Wandlers) (positiv weil Trigger auf Bezug)
const float emeterDischargeStopPower=-50;   // Trigger das Entladen abzubrechen (im normalfall 0 weil ich nicht aus dem Akku einspeisen will, etwas tolleranz gewähren bzw. differenz starttriggger entladen/tatsächlicher entladeleistung) (negativ weil Trigger auf Einspeisung)

// Batteriemessung (Leerlauf wie vom Akkuhersteller beschrieben)
const float battEmergencyStart=10;          // %Akku Ladug bei der die Ladung unabhängig von Solarleistung gestartet wird um Schaden am Akku zu verhindern
const float battFull=100;                   // %Akku bei der keine Ladung mehr gestartet wird (automatiklader, daher unkritisch)

const float battApplicableNight=30;         // %Akku die mindestens vorhanden sein muss um den Einspeisevorgang (neu)starten (wenn Unterbrochen) 
const float battApplicableDay=50;           // %Akku die mindestens vorhanden sein muss um den Einspeisevorgang (neu)starten (wenn Unterbrochen)

const float battStopDischarge=15;           // Entladevoragnag stoppen, während dem entladen funktioniet die Akkumessung leider nicht, zeigt immer weniger an. Tatsächlicher Wert im Standby nach dem Entladestop höher
                                            // Nach Abschaltung der Entladung springt der Wert sprunghaft, der tatsächliche Ladezustand kann erst wenige Minuten nach Entladestop über die Akkuspannung abschätzt werden.


// --------------------------------------------
// Sicherheitsabschaltung

bool Prg_Controller::CheckFailure() {
  // Abschaltung weil Akkufehler, BMS hat abgeschaltet (passiert ggf. schon bei 5%), Sicherung geflogen, hier können später noch weiter Bedingungen aufgenommen werden.
  // Achtung, greift diese Routine geht die Software auf Fehler, bedeutet es wird auch nicht mehr geladen. Manueller Eingriff nötig!
  // Diese Prüfung wird auf der aktuell aktiven Batteie auf der aktuell aktiven Batterie ausgeführt, nicht generell auf beiden

  if (!mod_IO.BattActiveValid()) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattActive, mod_IO.vBatt_active);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_activeProz);
    return true;
  } else {
    return false;
  }
}

// --------------------------------------------
// Hilfsfunktionen 

boolean Prg_Controller::isDay() {
  // Result True = Tag , Result False = Nacht
  // Achtung, der Controller hat immer Winterzeit
  if ( 
        (mod_Timer.runTime.h >= 8) && (mod_Timer.runTime.h <= 17) 
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

  float emeterPower = mod_EMeterClient.GetCurrentPower(false);  // < 0 Einspeisung | > 0 Bezug
  if ( emeterPower == 0 ) { return false; } // Fehler wenn genau 0
  delay(1); // Yield()

  if (  
        (mod_IO.vBatt_activeProz < battFull) && 
        (emeterPower < emeterChargePower)  &&
        (isDay() == true) 
     ) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattActive, mod_IO.vBatt_active);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_activeProz);
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

  mod_PowerMeter.GetCurrentPower(false);  
  if (mod_PowerMeter.lastPower < chargeDetectPower) {
    chargeEndCounter += 1;
    Serial.print("chargeEndCounter "); Serial.println(chargeEndCounter);
  }
  else {
    Serial.print("chargeEndCounter reset");
    chargeEndCounter = 0;
  }

  if ( 
        (chargeEndCounter > 10) ||
        (emeterPower > 0) || 
        (isDay() == false)
     ) { 
    mod_PowerMeter.GetCurrentPower(true);  
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

  if ( 
        (mod_IO.vBatt_activeProz <= battEmergencyStart) && 
        (isDay() == true) && (mod_Timer.runTime.h > 12)
     ) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattActive, mod_IO.vBatt_active);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_activeProz);
    return true;
  }
  else {
    return false;
  }
}

bool Prg_Controller::triggerStopChargeEmergency() {
  // Stoptrigger für das Notfall Laden des Akkus, egal ob die Sonne scheint oder nicht
  
  mod_PowerMeter.GetCurrentPower(false);  
  if (mod_PowerMeter.lastPower < chargeDetectPower) {
    chargeEndCounter += 1;
    Serial.print("chargeEndCounter "); Serial.println(chargeEndCounter);
  }
  else {
    Serial.print("chargeEndCounter reset");
    chargeEndCounter = 0;
  }

  if ( 
        (chargeEndCounter > 10) ||
        (isDay() == false) 
     ) {
    mod_PowerMeter.GetCurrentPower(true);  
    return true;
  }
  else {
    return false;
  }
}

// --------------------------------------------
// Entladetrigger
// Es muss unbedingt ein Akku mit BMS verwendet werden, welches den Entladevorgang überwacht und abschaltet bevor die Spannung zu weit sinkt

bool Prg_Controller::triggerStartDischarge() {
  // Starttrigger für das Entladen
  // Wenn die Batterie genügend Ladung hat und der Bezug/Verbrauch (einspeisung grad nicht aktiv) > der zu erwartenden Entladeleisung liegt
  // Zusätzlich kann über die Zeit geprüft werden, dass das erst Abends passiert damit der Akku nicht bereits am Tag entladen wird  (je nach dem ob man das will oder nicht, bei kleinem Akku nicht), Achtung: Start/Stop Bedingung gemeinsam anpassen

  float emeterPower = mod_EMeterClient.GetCurrentPower(false);  // < 0 Einspeisung | > 0 Bezug
  if ( emeterPower == 0 ) { return false; } // Fehler wenn genau 0
  delay(1); // Yield()

  if (isDay() == true) {

    // Am Tag den Einspeisemodus nur starten wenn 
    if ( 
         (mod_IO.vBatt_activeProz >= battApplicableDay) && 
         (emeterPower > emeterDischargePower)
      ) {
      mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattActive, mod_IO.vBatt_active);
      mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_activeProz);
      mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_EMeterPower, emeterPower);
      return true;
    }
    else {
      return false;
    }

  }
  else {

    // In der Nach den Einspeisemodus immer neu(starten) bis die Batterie die Mindestladung unterschritten hat
    if ( 
         (mod_IO.vBatt_activeProz >= battApplicableNight) && 
         (emeterPower > emeterDischargePower)
      ) {
      mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattActive, mod_IO.vBatt_active);
      mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_activeProz);
      mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_EMeterPower, emeterPower);
      return true;
    }
    else {
      return false;
    }

  }


}

bool Prg_Controller::triggerStopDischarge() {
  // Stoptrigger für das Entladen
  // gestoppt wenn entweder die EntladeSpannung unter das eingestellte limit geht (höher als vom BMS um den Akku zu schonen) oder der Energiebedarf in die Lieferung geht (einspeisung grad aktiv)
  // Zusätzlich kann die Zeit geprüft werden, also wenn die Nacht zu Ende ist. Achtung: Start/Stop Bedingung gemeinsam anpassen
  
  if ( 
        (mod_IO.vBatt_activeProz <= battStopDischarge)
    ) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattActive, mod_IO.vBatt_active);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_activeProz);
    return true;
  }

  delay(1); // Yield()
  float emeterPower = mod_EMeterClient.GetCurrentPower(false);  // < 0 Einspeisung | > 0 Bezug
  if ( emeterPower == 0 ) { return false; } // Fehler wenn genau 0
  delay(1); // Yield()
  if ( 
        (emeterPower < emeterDischargeStopPower) && (lastWRpwrset <= minWRpwrset+1) // Endladen abbrechen wenn ich im Lieferbereich bin, aber der Akku auch schon heruntergefahren ist
    ) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_EMeterPower, emeterPower);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattActive, mod_IO.vBatt_active);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_activeProz);
    return true;
  }

  return false;
}

void Prg_Controller::doPowerControl() {
  Serial.println("doPowerControl()");
  
  // EMeter Abfragen // < 0 Einspeisung | > 0 Bezug
  // Verhalten des EMeters (mittelung) in der Regelung berücksichtigen!
  float emeterPower = mod_EMeterClient.GetCurrentPower(false);  
  if ( emeterPower == 0 ) 
  { 
    // wenn genau 0 liegt entweder ein Fehler vor oder es passt perfekt (unwahrscheinlich), in beiden Fällen nichts tun
    Serial.println("doPowerControl() wird nicht ausgeführt da Einseisung/Bezug genau 0!");
    return; 
  } 
  delay(1); // Yield()

  // Trigger ist, ob das EMeter einen neuen Wert geliefert hat
  //if (emeterPower == lastEMeterpwr) {
  //  Serial.println("EMeter hat noch keinen neuen Wert geliefert, Skip!");
  //  detailsMsg = detailsMsg + ".";
  //  return;
  //} else {
  //  Serial.println("EMeter hat neuen Wert geliefert, Leistungsregelung wird ausgeführt");
  //}
  lastEMeterpwr = emeterPower;

  float wrPower = mod_BatteryWRClient.GetCurrentPower(false);
  if ( wrPower == 0 ) { 
    // Fehler wenn genau 0 bzw. speist nicht ein und damit kann ich nicht regeln
    // In Realität wird die Regelung nach dem Setzen der Initialleistung eh für n Minuten ausgesetzt bis der Wandler einspeist
    // Sollte dann wirklich noch keine Leistung eingespeist werden, ist eh was faul
    Serial.println("doPowerControl() wird nicht ausgeführt da WR Leistung unbekannt");
    return;  
  } 
  delay(1); // Yield()

  Serial.println("LastWRpower: " + String(lastWRpwrset) + " CurrentWRpower: " + String(wrPower) + " EMeter: " + String(emeterPower)); 

  if (emeterPower > 0 ) {
    // > 0 Bezug
    // Wechselrichterleistung um Bezug erhöhen (ist positiv)
    lastWRpwrset = wrPower + (emeterPower*0.7); 
  };
  if (emeterPower < 0) {
    // < 0 Lieferung
    // Wechselrichterkesitung um Lieferung verrringern (ist negativ)
    lastWRpwrset = wrPower + (emeterPower*0.7);
  }
  if (lastWRpwrset < minWRpwrset) { lastWRpwrset = minWRpwrset; };
  if (lastWRpwrset > maxWRpwrset) { lastWRpwrset = maxWRpwrset; }; 
  Serial.println("NewWRpower(new): " + String(lastWRpwrset));

  // neue Leistung ist jetzt in lastpwrset abgelegt, wie wird gesetzt. im wrPower und emeterpower hab ich noch die Werte von davon
  detailsMsg = "Leistung: " + String(lastWRpwrset) + "W  (Vorherige Leistung " + String(wrPower)+ "W  EMeter: "+String(emeterPower)+"W)";

  if (mod_BatteryWRClient.SetPowerLimit(lastWRpwrset)) {
    Serial.println("doPowerControl() ok");
  }
  else {
    Serial.println("doPowerControl() nok");
  }

}


// --------------------------------------------
// Servicefunktionen

String Prg_Controller::GetState() {
  switch (state) {
      // Fehlerzustand
      case State_Failure:
        return "F";
        break;
      // Standby Zustand        
      case State_Standby:
        return "S";
        break;
      // Aktive Zustände
      case State_Charge:      
        return "C";
        break;
      case State_ChargeEmergency:
        return "C";
        break;
      case State_Discharge:
        return "D";
        break;
      default:
        return "?";
        break;
  }
}

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

String Prg_Controller::GetDetailsMsg() {
  return detailsMsg;
}

// --------------------------------------------

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
  lastWRpwrset = 0; //defaultWRpwrset;
  lastEMeterpwr = 0; // 0 wäre egal
  pwrControlSkip = 10; //wegen der Wechselrichter Rampe nach dem Einschalten die ersten Minuten nicht regeln 
  detailsMsg = "";  // Meldung aus der Regelung
  chargeEndCounter = 0; // Ladeende erst nach mehreren Durchläufen ohne Ladestrom erkennen

  // Akkuzustände ins Protokoll schreiben
  mod_IO.Off();
  delay(1); // Yield()
  mod_IO.MeasureBatt12(true);
  delay(1); // Yield()
  mod_IO.MeasureBattActive(true);
  delay(1); // Yield()

  // Fehlerzustand gleich prüfen
  if ( (state != State_Failure) && CheckFailure() ) {
    Serial.println("CheckFailure");
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_SystemFailure,0);

    mod_IO.Off();
    
    state = State_Failure;
  } 

  Serial.println("prgController_Init() Done");
}

void Prg_Controller::Handle() {
  
  // die Entscheidungsstruktur für den Modus triggert einmal pro Minute
  // d.h. alles was hier innerhalb passiert, kann niemals schneller passieren, das ist wichtig!
  // Sicherheitsrelevante Funktionen müssen außerhalb bzw. durch Hardware/BMS/Ladecontroller etc. abgefangen werden
  if ( (mod_Timer.runTime.m != triggertime_bak) && (mod_IO.IsmanIOMode() == false) ) {
    triggertime_bak = mod_Timer.runTime.m;
    Serial.println("Congroller Trigger");

    // WifiCheck
    if (ModStatic_Wifi::CheckConnected() != true) {
      Serial.println("CheckConnected Fehlgeschlagen");
      mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_WifiErrorDetected,0);
      //
    }

    // Akkumessung (wird für die Fehlerprüfung und in den verschiedenen Stages für die jeweiligen Triggerfunktionen benötigt)
    delay(1); // Yield()
    mod_IO.MeasureBatt12(false);
    delay(1); // Yield()
    mod_IO.MeasureBattActive(false);
    delay(1); // Yield()

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

        if (mod_Timer.runTime.m == 0) {
          if (mod_Timer.runTime.h == 6) {
            // wenn sich der Akku im Standby befindet, akkustand loggen
            // das ist vor allem im interessant wenn der Akku mehrere Tage im Standby ist 
            delay(1); // Yield()
            mod_IO.MeasureBatt12(true);
            delay(1); // Yield()
            mod_IO.MeasureBattActive(true);
          }
        }

        if (triggerStatCharge()) {
          Serial.println("triggerStatCharge");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StartCharge,0);

          mod_IO.Charge();

          chargeEndCounter = 0; // zurücksetzen, hiermit wird gezählt damit bei Kurzer Unterbrechung nicht das Ladeende erkannt wird

          state = State_Charge;
        }
        if (triggerStatChargeEmergency()) {
          Serial.println("triggerStatChargeEmergency");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StartChargeEmergency,0);

          mod_IO.Charge();

          chargeEndCounter = 0; // zurücksetzen, hiermit wird gezählt damit bei Kurzer Unterbrechung nicht das Ladeende erkannt wird

          state = State_ChargeEmergency;
        }
        if (triggerStartDischarge()) {
          Serial.println("triggerStartDischarge");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StartDischarge,0);

          //lastWRpwrset = defaultWRpwrset; // Vorgabe, Wandler fährt eh erst mal Rampe
          // initial aktuelle Leistung einstellen, so kann ich ggf. peaks am Tag besser ausregeln
          lastEMeterpwr = mod_EMeterClient.GetCurrentPower(false);  // < 0 Einspeisung | > 0 Bezug
          lastWRpwrset= lastEMeterpwr; // in dem Falle positiv sonst hätte der Trigger nicht ausgelöst
          delay(1); // Yield()        

          pwrControlSkip = 10; // wegen der Wechselrichter Rampe nach dem Einschalten die ersten Minuten nicht regeln, 
          detailsMsg = "Leistung: " + String(lastWRpwrset)+"W (Initialleistung)";
          mod_BatteryWRClient.SetPowerLimit(lastWRpwrset);

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

        // Leistungsregelung (muss träger sein als die Messung und ggf. beim Einschalten Rampe des Wandlers)
        pwrControlSkip -= 1;
        if ( pwrControlSkip < 1) {
          //pwrControlSkip = 3; // ab jetzt in einem festen intervall regeln  
          pwrControlSkip = 0; // ab jetzt jedes mal regeln. Voraussetzung ist dass die messung mind. drei mal so schnell ist, also immer ein aktueller Messwert vorliegt
          doPowerControl();
        } else {
          Serial.print("PwrControlSkip "); Serial.println(pwrControlSkip);
          detailsMsg = detailsMsg + ".";
        }

        if (triggerStopDischarge()) {
          Serial.println("triggerStopDischarge");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StopDischarge,0);

          // zurück auf initialzustand
          lastWRpwrset = 0; // defaultWRpwrset;
          lastEMeterpwr = 0; // 0 wäre egal 
          detailsMsg = "";

          mod_IO.Off();
          state = State_Standby;
        }
        break;

    }

  }

}

// --------------------------------------------