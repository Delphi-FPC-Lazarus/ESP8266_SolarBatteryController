// Leistungsregler 

#pragma once

// Entladeleistung 
const float maxWRpwrset=300;                // Maximalwert für den Wechselrichter (achtung, je Eingangsspannung bringt er das eh nicht, max 250 effektiv)
const float maxWRpwrsetStart=200;           // Maximalwert für den Wechselrichter beim Starten des WR (wegen Akku, Spannungmessung und Entladeendeerkennung)
const float maxWRpwrsetLowBatt=200;         // Maximalwert für den Wechselrichter bei schwachem Akku (wegen Akku, Spannungmessung und Entladeendeerkennung)
const float minWRpwrset=10;                 // Minimalwert für den Wechselrichter

const float battLowDischarge=20;            // Akku schwach, während dem entladen funktioniet die Akkumessung leider nicht, zeigt immer weniger an.

// --------------------------------------------
class Mod_PowerControl {
  private:
    // gemerkte Leistung für Regelung, nur zur interne Verwendung
    float lastEMeterpwr;
    
    // zuletzt gesetzte WR Leistung aus Entladestart oder Leistungsregelung
    float lastWRpwrset;
    
    // Informationen aus der Regelung, wird auch in der Oberfläche angezeigt 
    String detailsMsg;

  public:
    // Leistungsregrlung
    void ResetPowerControl();
    void InitPowerControl();
    void DoPowerControl();

    void DisableWR();
    void EnableWR();

    float GetLastWRpwrset();
    String getDetailsMsg();

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void init();
    void handle();
};
Mod_PowerControl mod_PowerControl;

// ------------------------------------------
float Mod_PowerControl::GetLastWRpwrset() {
  return lastEMeterpwr;
}

String Mod_PowerControl::getDetailsMsg() {
  return detailsMsg;
}

// ------------------------------------------
// Leistungsregelung

void Mod_PowerControl::ResetPowerControl()
{
  lastWRpwrset = 0; //defaultWRpwrset;
  lastEMeterpwr = 0; // 0 wäre egal
  detailsMsg = "";  // Meldung aus der Regelung
}

void Mod_PowerControl::InitPowerControl() {
  
  // Initial wird die aktuelle Bezugsleistung sowie Korrekturwert verwendet, so kann ich ggf. peaks am Tag besser ausregeln
  lastEMeterpwr = mod_EMeterClient.getCurrentPower(false);  // < 0 Einspeisung | > 0 Bezug
  lastWRpwrset= lastEMeterpwr; // in dem Falle positiv sonst hätte der Trigger nicht ausgelöst
            
  // Begrenzen (Start)
  if (lastWRpwrset < minWRpwrset) { lastWRpwrset = minWRpwrset; };
  if (lastWRpwrset > maxWRpwrsetStart) { lastWRpwrset = maxWRpwrsetStart; };
  Serial.println("NewWRpower(new): " + String(lastWRpwrset));

  // WR Leistung initial Einstellen+Meldung
  delay(1); // Yield()
  detailsMsg = "Leistung: " + String(lastWRpwrset)+"W (Initialleistung)";
  Serial.println(detailsMsg);
  if (mod_BatteryWRClient.setPowerLimit(lastWRpwrset)) {
    Serial.println("setPowerLimit() ok");
  }
  else {
    Serial.println("setPowerLimit() nok");
  }
}

void Mod_PowerControl::DoPowerControl() {
  Serial.println("doPowerControl()");
  
  // EMeter Abfragen // < 0 Einspeisung | > 0 Bezug
  // Trägheit des EMeters (aggregationszeit) berücksichtigen!
  float emeterPower = mod_EMeterClient.getCurrentPower(false);  
  if ( abs(emeterPower) < 1 ) 
  { 
    // wenn genau 0 liegt entweder ein Fehler vor oder es passt perfekt (selten), in beiden Fällen nichts tun
    Serial.println("doPowerControl() wird nicht ausgeführt da Einseisung/Bezug zu gering!");
    detailsMsg = "Leistung: " + String(lastWRpwrset) + "W  (EMeter: "+String(emeterPower)+"W, keine Anpassung notwendig)";
    Serial.println(detailsMsg);
    return; 
  } 
  delay(1); // Yield()

  // Aktuelle Einspeiseleistung des Wechselrichters abfragen
  // Trägheit der Wechselrichterabfrage (Abfrageintervall) berücksichtigen
  // (Workaround wäre hier die letzte gesetzte Leistung zu verwenden, dass ist aber schlecht, da die Eingestellte Leistung nicht zwingend anliegt)
  float wrPower = mod_BatteryWRClient.getCurrentPower(false);
  if ( wrPower == 0 ) { 
    // Fehler wenn genau 0 bzw. speist nicht ein und damit kann ich nicht regeln
    // In Realität wird die Regelung nach dem Setzen der Initialleistung eh für n Minuten ausgesetzt bis der Wandler einspeist
    // Sollte dann wirklich noch keine Leistung eingespeist werden, ist eh was faul
    Serial.println("doPowerControl() wird nicht ausgeführt da WR Leistung unbekannt");
    detailsMsg = "Leistung: " + String(lastWRpwrset) + "W  (EMeter: "+String(emeterPower)+"W, WR Leistung konnte nicht ermittelt werden)";
    Serial.println(detailsMsg);
    return;  
  } 
  delay(1); // Yield()

  // Regler 
  // emeterPower ist der Fehlwert
  // > 0 Bezug: Wechselrichterleistung um Bezug erhöhen (ist positiv)
  // < 0 Lieferung: Wechselrichterkesitung um Lieferung verrringern (ist negativ)
  // wrPower ist die aktuelle Wechselrichter Leistung
  // Bei den Regelungsfaktoren ist das Verhalten von WR/DTU beim Setzen neuer Werte zu berücksichtigen
  // (manuelle Leistungsregelung "auf null" über die DTU vorher ausprobieren, 50% bis max 70% Messwertabweichung zustellen, 
  //  manchmal übersteuert der WR besonders bei starker positiver Leistungsänderung, das führt zum Schwingen der Regelung,
  //  also lieber vorsichtiger aber dafür schneller Regeln, das führt zu einm Besseren Ergebenis)
  float P = 0.5 * emeterPower;  // P-Anteil (langsame Annähreung)
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
  if (mod_BatteryWRClient.setPowerLimit(lastWRpwrset)) {
    Serial.println("setPowerLimit() ok");
  }
  else {
    Serial.println("setPowerLimit() nok");
  }

  // Vorherigen Fehlwert für nächsten Zyklus speichern
  lastEMeterpwr = emeterPower;

}

void Mod_PowerControl::DisableWR() {
  Serial.println("DisableWR");

  // Leistung runter
  if (mod_BatteryWRClient.setPowerLimit(10)) {
    Serial.println("setPowerLimit() ok");
  }
  else {
    Serial.println("setPowerLimit() nok");
  }

  // Abschalten
  mod_BatteryWRClient.setDisable();
}

void Mod_PowerControl::EnableWR() {
  Serial.println("EnableWR");

  // Leistung hier nicht ändern

  // Einschalten
  mod_BatteryWRClient.setEnable();
}

// ------------------------------------------
// Standard Init/Handler 

void Mod_PowerControl::init()
{
  Serial.println("modPowerControl_init()");
  
  ResetPowerControl();
}

void Mod_PowerControl::handle()
{
	// hier nichts tun
}

// ------------------------------------------
