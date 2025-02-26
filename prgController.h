// Automatik Steuerung

#pragma once

#define SOFTWARE_VERSION "2.12"

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
    bool SelectBatteryNotFull();
    bool SelectBatteryNotEmpty();

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
const int chargeDetectDelay=5;              // Erkennung muss entsprechend oft vorkommen 

// Entladeleistung 
const int dischargeStartRamp=10;            // wegen der Wechselrichter Rampe beim einschalten nach dem Einschalten die ersten Minuten nicht regeln 
const float maxWRpwrset=300;                // Maximalwert für den Wechselrichter (achtung, je Eingangsspannung bringt er das eh nicht, max 250 effektiv)
const float maxWRpwrsetStart=200;           // Maximalwert für den Wechselrichter beim Starten des WR (wegen Akku, Spannungmessung und Entladeendeerkennung)
const float maxWRpwrsetLowBatt=200;         // Maximalwert für den Wechselrichter bei schwachem Akku (wegen Akku, Spannungmessung und Entladeendeerkennung)
const float minWRpwrset=10;                 // Minimalwert für den Wechselrichter

const float emeterDischargePower=50;        // Trigger das Entladen begonnen werden kann (entspricht mind. Entladeleistung des Wandlers) (positiv weil Trigger auf Bezug)
const float emeterDischargeStopPower=-50;   // Trigger das Entladen abzubrechen (im normalfall 0 weil ich nicht aus dem Akku einspeisen will, etwas tolleranz gewähren bzw. differenz starttriggger entladen/tatsächlicher entladeleistung) (negativ weil Trigger auf Einspeisung)


// Batteriemessung (Leerlauf wie vom Akkuhersteller beschrieben)
const float battEmergencyStart=10;          // %Akku Ladug bei der die Ladung am Tag unabhängig von Solarleistung gestartet wird um Schaden am Akku zu verhindern
const float battFull=100;                   // %Akku bei der keine Ladung mehr gestartet wird (automatiklader, daher unkritisch)

const float battApplicable=30;              // %Akku die mindestens vorhanden sein muss um den Einspeisevorgang (neu)starten (wenn Unterbrochen) 
const float battLowDischarge=20;            // Akku schwach, während dem entladen funktioniet die Akkumessung leider nicht, zeigt immer weniger an.
const float battStopDischarge=10;           // Entladevoragnag stoppen, während dem entladen funktioniet die Akkumessung leider nicht, zeigt immer weniger an. Tatsächlicher Wert im Standby nach dem Entladestop höher
                                            // Nach Abschaltung der Entladung springt der Wert sprunghaft, der tatsächliche Ladezustand kann erst wenige Minuten nach Entladestop über die Akkuspannung abschätzt werden.


// --------------------------------------------
// Sicherheitsabschaltung

bool Prg_Controller::CheckFailure() {
  // Abschaltung weil Akkufehler, BMS hat abgeschaltet (passiert ggf. schon bei 5%), Sicherung geflogen, hier können später noch weiter Bedingungen aufgenommen werden.
  // Diese Prüfung wird auf der aktuell aktiven Batteie auf der aktuell aktiven Batterie ausgeführt, nicht generell auf beiden
  // Achtung, greift diese Routine geht die Software auf Fehler, bedeutet es wird auch nicht mehr geladen. Manueller Eingriff nötig!

  if (!mod_IO.BattActiveValid()) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Separator, 0);

    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattActive, mod_IO.vBatt_active);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_activeproz);
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

bool Prg_Controller::SelectBatteryNotFull() {

  if (mod_IO.Batt1Valid() && mod_IO.Batt2Valid()) {

    Serial.println("SelectBatteryNotFull() 2 Akku Betrieb");
    // 2 AKku Betrieb ->
    // Toggle je nach dem ob ein gerader oder ungerader Tag ist um die Akkunutzung besser zu verteilen. Die Funktion wird vom Ladetrigger aufgerufen, im Idealfall sowieso zweimal dann wäre es egal
    if (mod_Timer.runTime.d % 2 == 0) {
      Serial.println("Prüfung 1 dann 2");
      // erst Batt 1 dann Batt 2
      if (mod_IO.vBatt_1proz < battFull) {
        mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
        mod_IO.SelectBattActive(1);
        return true;
      } else {
        if (mod_IO.vBatt_2proz < battFull) {
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
          mod_IO.SelectBattActive(2);
          return true;
        }
      }
    } else {
      Serial.println("Prüfung 2 dann 1");
      // erst Batt 2 dann Batt 1
      if (mod_IO.vBatt_2proz < battFull) {
        mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
        mod_IO.SelectBattActive(2);
        return true;
      } else {
        if (mod_IO.vBatt_1proz < battFull) {
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
          mod_IO.SelectBattActive(1);
          return true;
        }
      }
    }
    // <-    

  } else {

    Serial.println("SelectBatteryNotFull() 1 Akku Betrieb");    
    // 1 Akku Betrieb ->
    if (mod_IO.Batt1Valid()) {
      // nur Batt 1
      if (mod_IO.vBatt_1proz < battFull) {
        mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
        mod_IO.SelectBattActive(1);
        return true;
      }
    }
    // <-

  }
  return false;

}

bool Prg_Controller::SelectBatteryNotEmpty() {
  
  if (mod_IO.Batt1Valid() && mod_IO.Batt2Valid()) {

    Serial.println("SelectBatteryNotEmpty() 2 Akku Betrieb");
    // 2 AKku Betrieb ->
    // Toggle je nach dem ob ein gerader oder ungerader Tag ist um die Akkunutzung besser zu verteilen. Die Funktion wird vom Ladetrigger aufgerufen, im Idealfall sowieso zweimal dann wäre es egal
    if (mod_Timer.runTime.d % 2 == 0) {
      Serial.println("Prüfung 1 dann 2");
      // erst Batt 1 dann Batt 2
      if (mod_IO.vBatt_1proz >= battApplicable) {
        mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
        mod_IO.SelectBattActive(1);
        return true;
      } else {
        if (mod_IO.vBatt_2proz >= battApplicable) {
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
          mod_IO.SelectBattActive(2);
          return true;
        }
      }
    } else {
      Serial.println("Prüfung 2 dann 1");
      // erst Batt 2 dann Batt 1
      if (mod_IO.vBatt_2proz >= battApplicable) {
        mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
        mod_IO.SelectBattActive(2);
        return true;
      } else {
        if (mod_IO.vBatt_1proz >= battApplicable) {
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
          mod_IO.SelectBattActive(1);
          return true;
        }
      }
    }
    // <-

  } else {

    Serial.println("SelectBatteryNotEmpty() 1 Akku Betrieb");    
    // 1 AKku Betrieb ->
    if (mod_IO.Batt1Valid()) {
      // nur Batt 1
      if (mod_IO.vBatt_1proz >= battApplicable) {
        mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
        mod_IO.SelectBattActive(1);
        return true;
      }
    }
    // <-

  }
  return false;
}

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
        (emeterPower < emeterChargePower)  &&
        (isDay() == true) 
     ) {
      
      if (SelectBatteryNotFull()) {
        // da sich ggf. die aktive Batterie geändert hat, aktive Batterie neu messen
        delay(1); // Yield()
        mod_IO.MeasureBattActive(false);
        delay(1); // Yield()

        mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattActive, mod_IO.vBatt_active);
        mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_activeproz);
        mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_EMeterPower, emeterPower);
        return true;
      } else {
        return false;
      }
  
  }
  else {
    return false;
  }
}

bool Prg_Controller::triggerStopCharge() {
  // Stoptrigger für das reguläre Laden des Akkus (activebattery)
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
        (chargeEndCounter > chargeDetectDelay) ||
        (emeterPower > 0) || 
        (isDay() == false)
     ) { 
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
    
    mod_PowerMeter.GetCurrentPower(true);  
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_EMeterPower, emeterPower);
    return true;
  }
  else {
    return false;
  }
}

bool Prg_Controller::triggerStatChargeEmergency() {
  // Starttrigger für Notfall Laden wenn der Akku zu weit runter ist, 
  // egal ob die Sonne scheint oder nicht wenn die Batteriespannung zu tief abgesackt ist
  // Dies wird einfach für beide Akkus nacheinander getan

  if ( 
        (mod_IO.Batt1Valid()) &&
        (mod_IO.vBatt_1proz <= battEmergencyStart) && 
        (isDay() == true) && (mod_Timer.runTime.h > 12)
     ) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Separator, 0);

    mod_IO.SelectBattActive(1);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBatt1, mod_IO.vBatt_1);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_1proz);
    return true;
  }

  if ( 
        (mod_IO.Batt2Valid()) &&
        (mod_IO.vBatt_2proz <= battEmergencyStart) && 
        (isDay() == true) && (mod_Timer.runTime.h > 12)
     ) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Separator, 0);

    mod_IO.SelectBattActive(2);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBatt2, mod_IO.vBatt_2);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_2proz);
    return true;
  }

  return false;
}

bool Prg_Controller::triggerStopChargeEmergency() {
  // Stoptrigger für das Notfall Laden des Akkus (activebattery)
  
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
        (chargeEndCounter > chargeDetectDelay) ||
        (isDay() == false) 
     ) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Separator, 0);

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

  if (emeterPower > emeterDischargePower) {

    if (SelectBatteryNotEmpty()) { 
      mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattActive, mod_IO.vBatt_active);
      mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_activeproz);
      mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_EMeterPower, emeterPower);
      return true;
    }
    else {
      return false;
    }

  }
  else {
    return false;
  }

}

bool Prg_Controller::triggerStopDischarge() {
  // Stoptrigger für das Entladen (activebattery)
  // gestoppt wenn entweder die EntladeSpannung unter das eingestellte limit geht (höher als vom BMS um den Akku zu schonen) oder der Energiebedarf in die Lieferung geht (einspeisung grad aktiv)
  // Zusätzlich kann die Zeit geprüft werden, also wenn die Nacht zu Ende ist. Achtung: Start/Stop Bedingung gemeinsam anpassen
  
  if ( 
        (mod_IO.vBatt_activeproz <= battStopDischarge)
    ) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Separator, 0);

    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattActive, mod_IO.vBatt_active);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_activeproz);
    return true;
  }

  delay(1); // Yield()
  float emeterPower = mod_EMeterClient.GetCurrentPower(false);  // < 0 Einspeisung | > 0 Bezug
  if ( emeterPower == 0 ) { return false; } // Fehler wenn genau 0
  delay(1); // Yield()
  if ( 
        (emeterPower < emeterDischargeStopPower) && (lastWRpwrset <= minWRpwrset+1) // Endladen abbrechen wenn ich im Lieferbereich bin, aber der Akku auch schon heruntergefahren ist
    ) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_Separator, 0);

    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_EMeterPower, emeterPower);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattActive, mod_IO.vBatt_active);
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_activeproz);
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

  // Aktuelle Einspeiseleistung des Wechselrichters abfragen
  float wrPower = mod_BatteryWRClient.GetCurrentPower(false);
  if ( wrPower == 0 ) { 
    // Fehler wenn genau 0 bzw. speist nicht ein und damit kann ich nicht regeln
    // In Realität wird die Regelung nach dem Setzen der Initialleistung eh für n Minuten ausgesetzt bis der Wandler einspeist
    // Sollte dann wirklich noch keine Leistung eingespeist werden, ist eh was faul
    Serial.println("doPowerControl() wird nicht ausgeführt da WR Leistung unbekannt");
    return;  
  } 
  delay(1); // Yield()

  // Regler 
  // emeterPower ist der Fehlwert
  // > 0 Bezug: Wechselrichterleistung um Bezug erhöhen (ist positiv)
  // < 0 Lieferung: Wechselrichterkesitung um Lieferung verrringern (ist negativ)
  // wrPower ist die aktuelle Wechselrichter Leistung
  float P = 0.7 * emeterPower;  // P-Anteil (langsame Annähreung)
  //float D = 0.3 * (emeterPower - lastEMeterpwr);  // D-Anteil (schnelle Korrektur)
  lastWRpwrset = wrPower + P; // + D; // Regelung

  // Begrenzen (regelung)
  if (lastWRpwrset < minWRpwrset) { lastWRpwrset = minWRpwrset; };
  if (lastWRpwrset > maxWRpwrset) { lastWRpwrset = maxWRpwrset; }; 
  if  (mod_IO.vBatt_activeproz <= battLowDischarge) {
    Serial.println("Low Battery");
    if (lastWRpwrset > maxWRpwrsetLowBatt) { lastWRpwrset = maxWRpwrsetLowBatt; }; 
  }
  Serial.println("NewWRpower(new): " + String(lastWRpwrset));

  // WR Leistung Einstellen+Meldung
  // Wert einstellen, neue Leistung ist jetzt in lastpwrset abgelegt, wie wird gesetzt. im wrPower und emeterpower hab ich noch die Werte von davor
  detailsMsg = "Leistung: " + String(lastWRpwrset) + "W  (Vorherige Leistung " + String(wrPower)+ "W  EMeter: "+String(emeterPower)+"W)";
  Serial.println(detailsMsg);
  if (mod_BatteryWRClient.SetPowerLimit(lastWRpwrset)) {
    Serial.println("doPowerControl() ok");
  }
  else {
    Serial.println("doPowerControl() nok");
  }

  // Vorherigen Fehlwert für nächsten Zyklus speichern
  lastEMeterpwr = emeterPower;

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
  pwrControlSkip = dischargeStartRamp; //wegen der Wechselrichter Rampe nach dem Einschalten die ersten Minuten nicht regeln 
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
    Serial.println("Controller Trigger");

    // WifiCheck
    if (ModStatic_Wifi::CheckConnected() != true) {
      Serial.println("CheckConnected Fehlgeschlagen");
      mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_WifiErrorDetected,0);
      //
    }

    // Akkumessung (wird für die Fehlerprüfung und in den verschiedenen Stages für die jeweiligen Triggerfunktionen benötigt deshalb hier abfragen)
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
            // Zusätzlich auf Akku 1 zurückschalten (Umschaltrelais abschalten, strom sparen)
            mod_IO.SelectBattActive(1);
          }
        }

        if (triggerStatCharge()) {
          Serial.println("triggerStatCharge");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StartCharge,0);

          mod_IO.Charge();

          chargeEndCounter = 0; // zurücksetzen, hiermit wird gezählt damit bei Kurzer Unterbrechung nicht das Ladeende erkannt wird

          state = State_Charge;
          break;
        }
        if (triggerStatChargeEmergency()) {
          Serial.println("triggerStatChargeEmergency");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StartChargeEmergency,0);

          mod_IO.Charge();

          chargeEndCounter = 0; // zurücksetzen, hiermit wird gezählt damit bei Kurzer Unterbrechung nicht das Ladeende erkannt wird

          state = State_ChargeEmergency;
          break;
        }
        if (triggerStartDischarge()) {
          Serial.println("triggerStartDischarge");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StartDischarge,0);

          // Hier sind wir im Bezug, sonst hätte er nicht getriggert
          // Initial wird die aktuelle Bezugsleistung Bezugsleistung sowie Korrekturwert verwendet, so kann ich ggf. peaks am Tag besser ausregeln
          // Der Wandler fährt eh erst mal Rampe
          lastEMeterpwr = mod_EMeterClient.GetCurrentPower(false);  // < 0 Einspeisung | > 0 Bezug
          lastWRpwrset= lastEMeterpwr; // in dem Falle positiv sonst hätte der Trigger nicht ausgelöst
          pwrControlSkip = dischargeStartRamp; // wegen der Wechselrichter Rampe nach dem Einschalten die ersten Minuten nicht regeln 

          // Begrenzen (Start)          
          if (lastWRpwrset < minWRpwrset) { lastWRpwrset = minWRpwrset; };
          if (lastWRpwrset > maxWRpwrsetStart) { lastWRpwrset = maxWRpwrsetStart; }; 
          Serial.println("NewWRpower(new): " + String(lastWRpwrset));

          // WR Leistung Einstellen+Meldung
          delay(1); // Yield()        
          detailsMsg = "Leistung: " + String(lastWRpwrset)+"W (Initialleistung)";
          Serial.println(detailsMsg);
          if (mod_BatteryWRClient.SetPowerLimit(lastWRpwrset)) {
            Serial.println("doPowerControl() ok");
          }
          else {
            Serial.println("doPowerControl() nok");
          }
          mod_IO.Discharge();

          state = State_Discharge;
          break;
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
          pwrControlSkip = 0; // ab jetzt ab jetzt in einem festen intervall (jedes mal) regeln. Voraussetzung ist dass die messung mind. drei mal so schnell ist, also immer ein aktueller Messwert vorliegt
          doPowerControl();
        } else {
          Serial.print("PwrControlSkip "); Serial.println(pwrControlSkip);
          detailsMsg = detailsMsg + ".";
        }

        if (triggerStopDischarge()) {
          Serial.println("triggerStopDischarge");
          mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_StopDischarge,0);

          // zurück auf initialzustand
          lastWRpwrset = 0;  // defaultWRpwrset;
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