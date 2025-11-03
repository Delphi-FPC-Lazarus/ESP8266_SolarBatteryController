// Automatik Steuerung

#pragma once

#define SOFTWARE_VERSION "2.35"

enum PrgState {
  State_Failure,          // system failure
  State_Standby,          // standby, hardware disconnected or manual mode
  State_Ready,            // ready
  State_Charge,           // charge battery
  State_ChargeEmergency,  // charge battery to prevent deep discharge
  State_Discharge         // discharge battery
};

// --------------------------------------------
class Prg_Controller {
  private:
    // Steuerung
    PrgState state;
    _runTime triggertime_control_bak;
    _runTime triggertime_regulation_bak;
    
    // Totzeit des WR nachcdfm Verbinden mit dem Netz die Leistungsregelung aussetzten und auf Initialleistung bleiben
    int pwrControlSkip;
    
    // Verzögerung der Ladeende Erkennung
    int chargeEndCounter;

    // Hilfsfunktionem
    bool checkFailure();
    bool isDay();
    bool selectBatteryNotFull();
    bool selectBatteryApplicable();
    bool isBatteryApplicable();

    // Triggerfunktionen
    bool triggerStartCharge();
    bool triggerStopCharge();

    bool triggerStartChargeEmergency();
    bool triggerStopChargeEmergency();

    bool triggerStartDischarge();
    bool triggerStopDischarge();

  public:
    String getState();
    String getStateString();
    String getDetailsMsg();

    void setState(PrgState newState, bool increasedstate);

    void setManualModeOn();
    void setManualModeOff();

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void init();
    void handle();
};
Prg_Controller prg_Controller;

// Allgemeines
const int startOfDay=8;                     // Tag Anfang, wird als zusätzliche Begrenzung für Ladezeiten verwendet
const int endOfDay=18;                      // Tag Ende, wird als zusätzliche Begrenzung für Ladezeiten verwendet

const int timetrig_pwrSafeStateReset = 10;  // Energiesparen bei längerem Standby, vorbereitend zurück auf Modus Standby
const int timetrig_pwrSafeRelaisReset = 11; // Energiesparen bei längerem Standby
const int timetrig_akkuLogMorning=8;        // Loggen unmittekbar bevor das Ladezeitfenster beginnt, hier ist der Akku mit hoher Wahrscheinlichkeit im Standy
const int timetrig_akkuLogEvening=17;       // Loggen am Abend, hier ist der Akku mit hoher Wahrscheinlichkeit im Standy

// Ladeleistung 500W
const float emeterChargePower=-500;         // Trigger das Laden begonnen werden kann (entspricht mind. Ladeleistung des Batterieladers) (negativ weil Trigger auf Einspeisung)
const float chargeDetectPower=300;          // Erkennung des Lademodus bzw. Erkennung des Ladeenedes
const int chargeDetectDelay=5;              // Erkennung muss entsprechend oft vorkommen 

// Entladeleistung 
const int dischargeStartRamp=10;            // wegen der Wechselrichter Rampe beim einschalten nach dem Einschalten die ersten Minuten nicht regeln 

const float emeterDischargePower=50;        // Trigger das Entladen begonnen werden kann (entspricht mind. Entladeleistung des Wandlers) (positiv weil Trigger auf Bezug)
const float emeterDischargeStopPower=-50;   // Trigger das Entladen abzubrechen (im normalfall 0 weil ich nicht aus dem Akku einspeisen will, etwas tolleranz gewähren bzw. differenz starttriggger entladen/tatsächlicher entladeleistung) (negativ weil Trigger auf Einspeisung)

// Batteriemessung (Leerlauf wie vom Akkuhersteller beschrieben)
const float battEmergencyStart=10;          // %Akku Ladug bei der die Ladung am Tag unabhängig von Solarleistung gestartet wird um Schaden am Akku zu verhindern
const float battFull=100;                   // %Akku bei der keine Ladung mehr gestartet wird (automatiklader, daher unkritisch)

const float battApplicable=30;              // %Akku die mindestens vorhanden sein muss um den Einspeisevorgang (neu)starten (wenn Unterbrochen) 
const float battStopDischarge=10;           // Entladevoragnag stoppen, während dem entladen funktioniet die Akkumessung leider nicht, zeigt immer weniger an. Tatsächlicher Wert im Standby nach dem Entladestop höher
                                            // Nach Abschaltung der Entladung springt der Wert sprunghaft, der tatsächliche Ladezustand kann erst wenige Minuten nach Entladestop über die Akkuspannung abschätzt werden.


// --------------------------------------------
// Sicherheitsabschaltung

bool Prg_Controller::checkFailure() {
  // Abschaltung weil Akkufehler, BMS hat abgeschaltet (passiert ggf. schon bei 5%), Sicherung geflogen, hier können später noch weiter Bedingungen aufgenommen werden.
  // Diese Prüfung wird auf der aktuell aktiven Batteie auf der aktuell aktiven Batterie ausgeführt, nicht generell auf beiden
  // Achtung, greift diese Routine geht die Software auf Fehler, bedeutet es wird auch nicht mehr geladen. Manueller Eingriff nötig!

  if (!mod_IO.isBattActiveValid()) {
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);

    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBattActive, mod_IO.vBatt_active);
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_activeproz);
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
        (mod_Timer.runTime.h >= startOfDay) && (mod_Timer.runTime.h <= endOfDay)
     ) {
     return true;
  } else {
    return false;
  } 
}

bool Prg_Controller::selectBatteryNotFull() {

  if (mod_IO.isBatt1Valid() && mod_IO.isBatt2Valid()) {

    Serial.println("selectBatteryNotFull() 2 Akku Betrieb");
    // 2 AKku Betrieb ->
    // Toggle je nach dem ob ein gerader oder ungerader Tag ist um die Akkunutzung besser zu verteilen. Die Funktion wird vom Ladetrigger aufgerufen, im Idealfall sowieso zweimal dann wäre es egal
    if (mod_Timer.runTime.d % 2 == 0) {
      Serial.println("Prüfung 1 dann 2");
      // erst Batt 1 dann Batt 2
      if (mod_IO.vBatt_1proz < battFull) {
        mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
        mod_IO.selectBattActive(1);
        return true;
      } else {
        if (mod_IO.vBatt_2proz < battFull) {
          mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
          mod_IO.selectBattActive(2);
          return true;
        }
      }
    } else {
      Serial.println("Prüfung 2 dann 1");
      // erst Batt 2 dann Batt 1
      if (mod_IO.vBatt_2proz < battFull) {
        mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
        mod_IO.selectBattActive(2);
        return true;
      } else {
        if (mod_IO.vBatt_1proz < battFull) {
          mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
          mod_IO.selectBattActive(1);
          return true;
        }
      }
    }
    // <-    

  } else {

    Serial.println("selectBatteryNotFull() 1 Akku Betrieb");    
    // 1 Akku Betrieb ->
    if (mod_IO.isBatt1Valid()) {
      // nur Batt 1
      if (mod_IO.vBatt_1proz < battFull) {
        mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
        mod_IO.selectBattActive(1);
        return true;
      }
    }
    // <-

  }
  return false;

}

bool Prg_Controller::selectBatteryApplicable() {
  
  if (mod_IO.isBatt1Valid() && mod_IO.isBatt2Valid()) {

    Serial.println("selectBatteryApplicable() 2 Akku Betrieb");
    // 2 AKku Betrieb ->
    // Toggle je nach dem ob ein gerader oder ungerader Tag ist um die Akkunutzung besser zu verteilen. Die Funktion wird vom Ladetrigger aufgerufen, im Idealfall sowieso zweimal dann wäre es egal
    if (mod_Timer.runTime.d % 2 == 0) {
      Serial.println("Prüfung 1 dann 2");
      // erst Batt 1 dann Batt 2
      if (mod_IO.vBatt_1proz >= battApplicable) {
        mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
        mod_IO.selectBattActive(1);
        return true;
      } else {
        if (mod_IO.vBatt_2proz >= battApplicable) {
          mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
          mod_IO.selectBattActive(2);
          return true;
        }
      }
    } else {
      Serial.println("Prüfung 2 dann 1");
      // erst Batt 2 dann Batt 1
      if (mod_IO.vBatt_2proz >= battApplicable) {
        mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
        mod_IO.selectBattActive(2);
        return true;
      } else {
        if (mod_IO.vBatt_1proz >= battApplicable) {
          mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
          mod_IO.selectBattActive(1);
          return true;
        }
      }
    }
    // <-

  } else {

    Serial.println("selectBatteryApplicable() 1 Akku Betrieb");    
    // 1 AKku Betrieb ->
    if (mod_IO.isBatt1Valid()) {
      // nur Batt 1
      if (mod_IO.vBatt_1proz >= battApplicable) {
        mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
        mod_IO.selectBattActive(1);
        return true;
      }
    }
    // <-

  }
  return false;
}

bool Prg_Controller::isBatteryApplicable() {
  
  if (mod_IO.isBatt1Valid() && mod_IO.isBatt2Valid()) {
    return ( (mod_IO.vBatt_1proz >= battApplicable) || (mod_IO.vBatt_2proz >= battApplicable) );
  }
  else {
    if (mod_IO.isBatt1Valid()) {
      return (mod_IO.vBatt_1proz >= battApplicable);
    } else {
      return false;
    }
  }
}


// --------------------------------------------
// Ladetrigger
// Es muss unbedingt ein Autmatik Lader sowie ein Akku mit BMS verwendet werden der den Ladevorgang für den Akku automatisch regelt und bei der entsprechenden Ladeschlussspannung abschaltet

bool Prg_Controller::triggerStartCharge() {
  // Starttrigger für das reguläre Laden des Akkus
  // Wenn die Batterie nicht voll ist und genügend Überschusseinspeisung zur Verfügung steht (in dem Zustand wird nicht geladen, also Einspeisung/Überschuss muss mindestens Ladeleistung sein)
  // ggf. noch Zeitbegrenzung das er nicht zu früh anfängt wegen Geräuschentwicklung, technisch ist das nicht nötig

  float emeterPower = mod_EMeterClient.getCurrentPower(false);  // < 0 Einspeisung | > 0 Bezug
  if ( emeterPower == 0 ) { return false; } // Fehler wenn genau 0
  delay(1); // Yield()

  if (  
        (emeterPower < emeterChargePower)  &&
        (isDay() == true) 
     ) {
      
      if (selectBatteryNotFull()) {
        // da sich ggf. die aktive Batterie geändert hat, aktive Batterie neu messen
        delay(1); // Yield()
        mod_IO.measureBattActive(false);
        delay(1); // Yield()

        mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBattActive, mod_IO.vBatt_active);
        mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_activeproz);
        mod_Logger.add(mod_Timer.runTimeAsString(),logCode_EMeterPower, emeterPower);
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
  float emeterPower = mod_EMeterClient.getCurrentPower(false);  // < 0 Einspeisung | > 0 Bezug
  if ( emeterPower == 0 ) { return false; } // Fehler wenn genau 0
  delay(1); // Yield()

  mod_PowerMeter.getCurrentPower(false);  
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
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
    
    mod_PowerMeter.getCurrentPower(true);  
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_EMeterPower, emeterPower);
    return true;
  }
  else {
    return false;
  }
}

bool Prg_Controller::triggerStartChargeEmergency() {
  // Starttrigger für Notfall Laden wenn der Akku zu weit runter ist, 
  // egal ob die Sonne scheint oder nicht wenn die Batteriespannung zu tief abgesackt ist
  // Dies wird einfach für beide Akkus nacheinander getan

  if ( 
        (mod_IO.isBatt1Valid()) &&
        (mod_IO.vBatt_1proz <= battEmergencyStart) && 
        (isDay() == true) && (mod_Timer.runTime.h > 12)
     ) {
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);

    mod_IO.selectBattActive(1);
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBatt1, mod_IO.vBatt_1);
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_1proz);
    return true;
  }

  if ( 
        (mod_IO.isBatt2Valid()) &&
        (mod_IO.vBatt_2proz <= battEmergencyStart) && 
        (isDay() == true) && (mod_Timer.runTime.h > 12)
     ) {
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);

    mod_IO.selectBattActive(2);
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBatt2, mod_IO.vBatt_2);
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_2proz);
    return true;
  }

  return false;
}

bool Prg_Controller::triggerStopChargeEmergency() {
  // Stoptrigger für das Notfall Laden des Akkus (activebattery)
  
  mod_PowerMeter.getCurrentPower(false);  
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
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);

    mod_PowerMeter.getCurrentPower(true);  
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

  float emeterPower = mod_EMeterClient.getCurrentPower(false);  // < 0 Einspeisung | > 0 Bezug
  if ( emeterPower == 0 ) { return false; } // Fehler wenn genau 0
  delay(1); // Yield()

  if (emeterPower > emeterDischargePower) {

    if (selectBatteryApplicable()) { 
      // da sich ggf. die aktive Batterie geändert hat, aktive Batterie neu messen
      delay(1); // Yield()
      mod_IO.measureBattActive(false);
      delay(1); // Yield()

      mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBattActive, mod_IO.vBatt_active);
      mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_activeproz);
      mod_Logger.add(mod_Timer.runTimeAsString(),logCode_EMeterPower, emeterPower);
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
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);

    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBattActive, mod_IO.vBatt_active);
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_activeproz);
    return true;
  }

  delay(1); // Yield()
  float emeterPower = mod_EMeterClient.getCurrentPower(false);  // < 0 Einspeisung | > 0 Bezug
  if ( emeterPower == 0 ) { return false; } // Fehler wenn genau 0
  delay(1); // Yield()

  // Endladen abbrechen wenn ich im Lieferbereich bin (mit Toleranz*)
  // Aber erst wenn der WR durch die Regelung schon heruntergefahren ist (mit Toleranz*) damit nicht bei kurzer Lastverminderung der Entladevorgang abbricht und komplett neu starten muss.
  // Dies ist nötig, da ich bei schneller Laständerung schnell in den Lieferbereich komme, dann soll er runterregeln und nicht gleich abschalten.
  // Dies macht das Abschalten natürlich Träger, das ist aber gewollt.
  // Allerdings greift damit die Abschaltung im Lieferbereich nicht in der Initalisierungsphase des WR, da dort die Leistung nie heruntergefahren wird
  if ( 
        (emeterPower < emeterDischargeStopPower) && (mod_PowerControl.GetLastWRpwrset() <= minWRpwrset+10) 
    ) {
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);

    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_EMeterPower, emeterPower);
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBattActive, mod_IO.vBatt_active);
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_VBattProz, mod_IO.vBatt_activeproz);
    return true;
  }

  return false;
}


// --------------------------------------------
// Servicefunktionen

String Prg_Controller::getState() {
  switch (state) {
      // Fehlerzustand
      case State_Failure:
        return "F";
        break;
      // Standby Zustand        
      case State_Standby:
        return "S";
        break;
      case State_Ready:
        return "R";
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

String Prg_Controller::getStateString() {
  switch (state) {
      // Fehlerzustand
      case State_Failure:
        return "<font color=red>Systemfehler</font>";
        break;
      // Standby Zustand        
      case State_Standby:
        return "Standby";
        break;
      case State_Ready:
        return "Bereit";
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

String Prg_Controller::getDetailsMsg() {
 
  String detailsMsg = "";
  switch (state) {
    case State_Ready:
      if (pwrControlSkip > 0) {
        detailsMsg = "(Wechselrichter Rampe " + String(pwrControlSkip) + ")";
      }
      break;
    case State_Discharge:
      detailsMsg = mod_PowerControl.getDetailsMsg();
      if (pwrControlSkip > 0) {
        detailsMsg = detailsMsg + " (keine Regelung " + String(pwrControlSkip) + ")";
      }
      break;
  }
  
  return detailsMsg;
}

// --------------------------------------------
// Funktionen für den Statuswechsel, 
// der Status darf generell nur über diese Funktionen gewechselt werden damit Hardwarzustand und Status übereinstimmen
void Prg_Controller::setState(PrgState newState, bool increasedstate) {
  // neuen Status übernehmen
  state = newState;
  // Hardware entsprechend neuem Status schalten
  switch (newState ) {
    case State_Failure:
      mod_Logger.add(mod_Timer.runTimeAsString(),logCode_SystemFailure,0);
      mod_IO.setOff();
      
      break;
    case State_Standby:
      mod_Logger.add(mod_Timer.runTimeAsString(),logCode_StateStandby,0);
      mod_IO.setOff();
            
      break;	
  	case State_Ready:
      // Das Entladen muss immer über diesen Status laufen
      mod_Logger.add(mod_Timer.runTimeAsString(),logCode_StateReady,0);
      // Wechselrichter deaktivieren und Leistungsvorgabe auf einen geringen Wert setzen
      mod_PowerControl.DisableWR();
      // Regelung zurück auf initialzustand
      mod_PowerControl.ResetPowerControl();

      if ( increasedstate ) {
        // Wechsel von Standby auf Ready

        // Wechselrichter mit dem Netz verbinden
        mod_IO.setDischarge();

        // Der Wandler muss nach dem verbinden mit dem Netz eine Rampe für die Leistungsbegrenzung durchführen
        // Diese gilt aber offenbar egal ob der Wandler einspeist oder nicht. Ob das ein Bug oder ein umgehen der Vorschriften ist, ist unbekannt
        pwrControlSkip = dischargeStartRamp;
      } else {
        // Wechsel von Discharge zurück auf Ready

        // Wechselrichter bleibt mit dem Netz verbunden
        // Rampe bleibt, sofern aktiv oder auch nicht, unverändert
      }

      break;
    case State_Charge:
      mod_Logger.add(mod_Timer.runTimeAsString(),logCode_StartCharge,0);
      mod_IO.setCharge();
      chargeEndCounter = 0; // zurücksetzen, hiermit wird gezählt damit bei Kurzer Unterbrechung nicht das Ladeende erkannt wird

      break;	
  	case State_ChargeEmergency:
      mod_Logger.add(mod_Timer.runTimeAsString(),logCode_StartChargeEmergency,0);
      mod_IO.setCharge();
      chargeEndCounter = 0; // zurücksetzen, hiermit wird gezählt damit bei Kurzer Unterbrechung nicht das Ladeende erkannt wird

      break;
  	case State_Discharge:
      mod_Logger.add(mod_Timer.runTimeAsString(),logCode_StartDischarge,0);
      // Wechselrichter aktivieren (aktiviert bei geringer Leistungsvorgabe)

      // Wechselrichter Leistung setzen
      // Initial wird die aktuelle Bezugsleistung verwendet, hier darf keine WR Leistung verwendet werden da i.d.R. deaktiviert
      // Je nach dem, ob das unmittelbar nach dem durchlaufen des Bereitschaftsstatus passiert, 
      // ist ggf. noch die Sperrzeit für die Regelung aktiv oder auch nicht. 
      // Im letzteren Fall wird im Regelintervall die Leistung im Anschluss sofort wieder angepasst. 
      mod_PowerControl.InitPowerControl();

      delay(1000); // der WR oder die DTU haben sonst manchmal Probleme

      mod_PowerControl.EnableWR();

      break;
	}
}

void Prg_Controller::setManualModeOn() {
  mod_IO.setManIOModeOn();
  setState(State_Standby, false);
}

void Prg_Controller::setManualModeOff() {
  mod_IO.setManIOModeOff();
  setState(State_Standby, false);
}

// --------------------------------------------
// Standardfunktionen

void Prg_Controller::init() {
  Serial.println("prgController_init()");
  
  // Initialzustand
  triggertime_control_bak = mod_Timer.runTime;
  triggertime_regulation_bak = mod_Timer.runTime;
  
  state = State_Standby;
  
  pwrControlSkip = 0;   //wegen der Wechselrichter Rampe nach dem Einschalten die ersten Minuten nicht regeln, wird dann im state gesetzt  
  chargeEndCounter = 0; // Ladeende erst nach mehreren Durchläufen ohne Ladestrom erkennen, wird dann im state gesetzt

  mod_IO.setOff();
  delay(1); // Yield()
  mod_IO.selectBattActive(1);

  // Akkuzustände ins Protokoll schreiben
  delay(1); // Yield()
  mod_IO.measureBatt12(true);

  // Aktuive Batterie ermittel, dies ist für den Betrieb wichtig 
  delay(1); // Yield()
  mod_IO.measureBattActive(true);
  delay(1); // Yield()

  // Fehlerzustand gleich prüfen
  if ( (state != State_Failure) && checkFailure() ) {
    Serial.println("checkFailure");

    setState(State_Failure, false);
  } 

  Serial.println("prgController_init() Done");
}

void Prg_Controller::handle() {

  if (mod_IO.isManIOMode() == false) {
    // Sicherheitsrelevante Funktionen müssen außerhalb der Folgenden Zeitsteuerungen bzw. durch Hardware/BMS/Ladecontroller etc. abgefangen werden

    // ZeitTrigger Steuerung (träge)
    // die Entscheidungsstruktur für den Modus triggert einmal pro Minute
    // d.h. alles was hier innerhalb passiert, kann niemals schneller passieren, das ist wichtig um die Hardware (relais) zu schonen
    if ( mod_Timer.runTime.m != triggertime_control_bak.m ) {
      triggertime_control_bak = mod_Timer.runTime;
      Serial.println("Trigger Controller");

      // WifiCheck
      if (ModStatic_Wifi::checkConnected() != true) {
        Serial.println("checkConnected Fehlgeschlagen");
        mod_Logger.add(mod_Timer.runTimeAsString(),logCode_WifiErrorDetected,0);
        //
      }

      // Akkumessung (wird für die Fehlerprüfung und in den verschiedenen Stages für die jeweiligen Triggerfunktionen benötigt deshalb hier abfragen)
      delay(1); // Yield()
      mod_IO.measureBatt12(false);
      delay(1); // Yield()
      mod_IO.measureBattActive(false);
      delay(1); // Yield()

      // Immer Fehlerprüfung aufrufen, in jedem Status außer wenn ich bereits im System failure status bin, dann ist eh alles tot
      if ( (state != State_Failure) && checkFailure() ) {
        Serial.println("checkFailure");
        setState(State_Failure, false);
      }

      // Totzeit für die Leistungsregelung wegen Rampe unmittelbar nach Verbinden mit dem Netz
      if (pwrControlSkip > 0) {
        pwrControlSkip -= 1;
        if ( pwrControlSkip < 1) {
          // ab jetzt ab jetzt in einem festen intervall
          // Aufgruf der Leistungsregelung ausgelagert für schnellere Reaktion, Zeittrigger Regelung 
          pwrControlSkip = 0; 
        } else {
          Serial.print("PwrControlSkip "); Serial.println(pwrControlSkip);
        }
      }

      switch (state) {
        // Fehlerzustand
        case State_Failure:
          Serial.println("State_Failure");
          // nichts mehr
          break;

        // Standby
        case State_Standby:
          Serial.println("State_Standby");

          // Dieser Status ist nur zur Initialisierung, übergang vom Laden 
          // sowie längerer Ruhephase und dem manuellen Modus da

          // wenn sich der Akku in diesem Modus befindet, nachts auf Akku 1 zurückschalten
          // (Umschaltrelais abschalten, strom sparen wenn der Akku mehrere Tage im Standby ist)
          // Dies mache ich absichtlich zu einem Zeitpunkt in dem der Akku im normalen Betrieb nicht im Standby um unnötige Schaltzyklen zu vermeiden
          // damit greift dies i.d.R.
          if (mod_Timer.runTime.m == 0) {
            if (mod_Timer.runTime.h == timetrig_pwrSafeRelaisReset) {
              Serial.println("Relais Reset im Standby (Stromsparen)");
              mod_IO.selectBattActive(1);
            }
          }

          // wenn sich der Akku in diesem Modus befindet, akkustand loggen
          // es ist nicht sichergestellt, dass dies immer passiert
          // ggf. interessant zum Feststellen der Akkustände oder auch wenn der Akku mehrere Tage im Standby ist
          if (mod_Timer.runTime.m == 0) {
            if ( (mod_Timer.runTime.h == timetrig_akkuLogMorning) || (mod_Timer.runTime.h == timetrig_akkuLogEvening) ) {
              Serial.println("Akkuzustand loggen");
              mod_Logger.add(mod_Timer.runTimeAsString(),logCode_Separator, 0);
              delay(1); // Yield()
              mod_IO.measureBatt12(true);
            }
          }

          // Prüfe Trigger für Statuswechsel

          // Wechsel in den Ladezustand
          if (triggerStartCharge()) {
            Serial.println("triggerStartCharge");
            setState(State_Charge, true);
            break;
          }
          
          // Wechsel in den Ladezustand (Tiefentladeschutz)
          if (triggerStartChargeEmergency()) {
            Serial.println("triggerStartChargeEmergency");
            setState(State_ChargeEmergency, true);
            break;
          }

          // Wenn mindestens eine Batterie noch Energie hat, direkt in den Status Bereitschaft wechseln
          // von dort aus kann jederzeit der Entlademodus ohne Startrampe angefahren werden
          if ((isBatteryApplicable()) ) {
            // Wechseln in den Entlademodus immer über den Bereitschaftzustand gehen, hier vorsorglich damit schneller bereit
            setState(State_Ready, true);
            break;
          }

          break;

        // Bereit
        case State_Ready:
          Serial.println("State_Ready");

          // Prüfe Trigger für Statuswechsel
          
          // Wechsel in den Ladezustand
          if (triggerStartCharge()) {
            Serial.println("triggerStartCharge");
            setState(State_Charge, true);
            break;
          }
          
          // Wechsel in den Ladezustand (Tiefentladeschutz)
          if (triggerStartChargeEmergency()) {
            Serial.println("triggerStartChargeEmergency");
            setState(State_ChargeEmergency, true);
            break;
          }
          
          // Wechsel in den Entladezustand
          if (triggerStartDischarge()) {
            Serial.println("triggerStartDischarge");
            // Wechsel in den Entlademodus (entladung aktiv) nur hier aus dem Bereitschaftsmodus
            setState(State_Discharge, true); 
            break;
          }
          
          // Wenn die Akkus leer sind, nachts auf Status Standby zurückwechseln
          if (mod_Timer.runTime.m == 0) {
            if (mod_Timer.runTime.h == timetrig_pwrSafeStateReset) {
              Serial.println("Prüfe Statuswechsel zurück auf Standby");
              if (!isBatteryApplicable()) {
                Serial.println("Statuswechsel zurück auf Standby");
                setState(State_Standby, false);
                break;
              }
            }
          }          

          break;

        // Laden
        case State_Charge:
          Serial.println("State_Charge");

          // Prüfe Trigger für Statuswechsel
          
          // Ladeende
          if (triggerStopCharge()) {
            Serial.println("triggerStopCharge");
            mod_Logger.add(mod_Timer.runTimeAsString(),logCode_StopCharge,0);
            setState(State_Standby, false);
            break;
          }

          break;

        // Laden (Tiefentladeschutz)
        case State_ChargeEmergency:
          Serial.println("State_ChargeEmergency");

          // Prüfe Trigger für Statuswechsel

          // Ladeende 
          if (triggerStopChargeEmergency()) {
            Serial.println("triggerStopChargeEmergency");
            mod_Logger.add(mod_Timer.runTimeAsString(),logCode_StopChargeEmergency,0);
            setState(State_Standby, false);
            break;
          }

          break;

        // Einspeisen
        case State_Discharge:
          Serial.println("State_Discharge");

          // Prüfe Trigger für Statuswechsel

          // Enthladestop trigger auch während der initialisierungsphase, siehe Bedingung innerhalb der Funktion deshalb greift es nicht
          if (triggerStopDischarge()) {
            Serial.println("triggerStopDischarge");
            mod_Logger.add(mod_Timer.runTimeAsString(),logCode_StopDischarge,0);
            setState(State_Ready, false);

            break;
          }

          break;

      } // switch state

    } // Ende Zeittrigger Steuerung

    // Zeittrigger Regelung (Mittelschnell)
    // Die Regelung läuft schneller als die Steuerung. Allerdings muss die Regelung immer entsprechend langsamer als die Messdatenerfassung laufen
    // Siehe entsprechenden Hinweis
    if ( (mod_Timer.runTime.s - triggertime_regulation_bak.s) >= 10 || mod_Timer.runTime.m != triggertime_regulation_bak.m) {
      triggertime_regulation_bak = mod_Timer.runTime;
      //Serial.println("Trigger Regulation");

      switch (state) {
        // Einspeisen
        case State_Discharge:
 
          // Leistungsregelung
          // Muss zwingend Träger sein als die E-Meter/Stromzähler Messung sein (dies ist eine Einstellung im ext vzlogger) 
          // Muss zwingend Träger sein als die über die DTU Abgefragte WR Leistung (dies ist eine Einstellung in der ext DTU)
          // Hintergrund ist, da der Wechselrichter immer differenziell zum Aktuellen WR Leistung abhängig von Fehlerwert = Energiemeter gesetzt wird
          // Zusätzlich muss das Regelverhalten des Wandlers selbst berücksichtigt werden, bei großer Leistungsänderung liegt der auch manchmal im ersten Moment stark daneben
          // Zusätzlich muss beim Einschalten des Wandlers dessen Rampe beachtet werden
          if ( pwrControlSkip == 0 ) {
            Serial.println("State_Discharge Trigger Regulation");
            mod_PowerControl.DoPowerControl();
          }
          break;
      }

    } // Ende Zeittrigger Regelung

  } // 
  
}

// --------------------------------------------