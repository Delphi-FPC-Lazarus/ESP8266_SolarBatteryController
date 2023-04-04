// Lokaler - offline - Laufzeitzähler mit basierend auf millis() (incl. Überlauferkennung) 
// Optional mit automatischer Synchronisation via NTP (inital/täglich) damit die Uhrzeit stimmt.
// Laufzeit in Tage Stunden Minuten Sekunden, Datum ist nicht vorgesehen. 
//
// Die Konstruktion ist relativ unempfindlich gegenüber unregelmäßigen Aufruf.
// wenn man die Sekunden nicht benötigt reicht wenn calRunTime() aller paar Sekunden aufgerufen wird (bzw. stört sich nicht dran wenn er nicht jede sekunde aufgerufen wird) 
// wenn kann die Sekunden benötigt, sollte der Aufruf in der Mainloop hängen.

#pragma once

#include "modNTPClient.h" 

// ------------------------------------------
struct _runTime {
  //ntp update
  unsigned long ntp_lastdaysyn;
  unsigned long ntp_millisoffset;
  // lokaler timer update
  unsigned long lastmillis;
  // zeit
  long d;
  byte h;
  byte m;
  byte s;
};

// --------------------------------------------
class Mod_Timer {
  private:
    void initRunTime();
    void calcRunTime();
    void syncFromNTP();
  public:
    // Laufzeitbereitstellung für den externen Zugriff
    _runTime runTime;
    // Laufzeitbereitstellung (Zeitstempel) für den Logger
    String runTimeAsString();

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void Init();
    void Handle();
};
Mod_Timer mod_Timer;

// ------------------------------------------

String Mod_Timer::runTimeAsString() {
  String resultString = "";
  if (runTime.d < 10)    {resultString += "0"+String(runTime.d)+" ";}	else {resultString += String(runTime.d)+" ";};      //0000
  if (runTime.h < 10)    {resultString += "0"+String(runTime.h);}   	  else {resultString += String(runTime.h);};        //00
  if (runTime.m < 10)    {resultString += "0"+String(runTime.m);}   	  else {resultString += String(runTime.m);};        //00
  if (runTime.s < 10)    {resultString += "0"+String(runTime.s);}       else {resultString += String(runTime.s);};        //00
  return resultString;
}

// ------------------------------------------
// interne Timer Funktionalität

void Mod_Timer::initRunTime() {
  runTime.ntp_lastdaysyn = 0;       // preset, wird eh beim Start synchronisiert
  runTime.ntp_millisoffset = 0;     // offset, wird der synchronisation gesetzt

  runTime.lastmillis = millis();    // jetzt (das ist i.d.R. kurz bis wenige Sekunden nach dem conroller start je nach dem in welcher Reihenfolge initialisiert wird)
  runTime.d = 0;
  runTime.h = 0;
  runTime.m = 0;
  runTime.s = 0;
}

void Mod_Timer::syncFromNTP() {
  Serial.println("modTimer_syncFromNTP()");
  // lokalen Laufzeittimer mit NTP Zeit initialisieren (Stunden Minuten Sekunden)
  mod_NTPClient.Update();

  runTime.lastmillis = millis();   
  // runTime.d =  // nicht ändern, mit dem zähle ich die Betriebstage
  runTime.h = mod_NTPClient.NTPhour;
  runTime.m = mod_NTPClient.NTPminute;
  // runTime.s nicht setzen weil ich die Sekunden nicht zähle sonder immmer ausrechne
  // stattdessen ntp_millisoffset setzen, lastmillis muss immer < millis sein sonst greift die Überlauferkennung
  runTime.ntp_millisoffset = mod_NTPClient.NTPsecond*1000;  
  Serial.print(runTime.h);Serial.print(':');Serial.print(runTime.m);Serial.print(" -> ntp_millisoffset ");Serial.println(runTime.ntp_millisoffset);
}

void Mod_Timer::calcRunTime() {
  // Achtung Datentyp unsigned long  
  //Serial.print("Debug: millis():"); Serial.print(millis()); Serial.print(" runTime.lastmillis:"); Serial.print(runTime.lastmillis); Serial.print(" "); Serial.println("");
  if (millis() > runTime.lastmillis) {
    // Minuten hochzählen, immer wenn eine Minute um ist 
    // damit als Überlauf Stunden und Tage bedienen
    if ((millis()-runTime.lastmillis+runTime.ntp_millisoffset) > (60*1000)) {
      Serial.println("Minutenwechsel");
      runTime.m += 1;
      if (runTime.m >= 60) {
        runTime.h += 1;
        runTime.m = 0;
      }
      if (runTime.h >= 24) {
        Serial.println("Stundenwechsel");
        runTime.d += 1;
        runTime.h = 0;
      }

      // lastmillis nicht auf millis() setzen sondern Rechnen, da nicht davon auszugehen ist das diese Funktion genau auf Punkt aufgerufen wurde
      runTime.lastmillis += 60*1000 - runTime.ntp_millisoffset;  
      //Serial.print("gerechneter Zeitstempel:"); Serial.println(runTime.lastmillis);
      if (runTime.lastmillis > millis()) { // lastmillis darf nie > milis() sein wegen überlauferkennung. Sollte bei der Rechnung nie der Fall sein aber falls doch... 
        //Serial.print("Korrektur:"); Serial.println(runTime.lastmillis);
        runTime.lastmillis = millis(); 
      }
      runTime.ntp_millisoffset = 0;  // ntp offset zurücksetzen
    }
  
    // Sekunden einfach aus der laufenden Minute ausrechnen 
    runTime.s = (millis()-runTime.lastmillis+runTime.ntp_millisoffset) / 1000;
  } 
  // Überlauf erkennen
  if (runTime.lastmillis > millis()) {
    Serial.println("millis() Überlauf erkannt");
    runTime.lastmillis = millis();
  }

  // Resynchronisation
  if ( (runTime.d > runTime.ntp_lastdaysyn) && (runTime.h == 3) && (runTime.m == 00) ) { // zu einem definierten Zeitpunkt mitten in der Nacht, keinesfalls direkt zum Tageswechsel
    Serial.println("modTimer Resynchronisation");
    runTime.ntp_lastdaysyn = runTime.d;
    syncFromNTP();
  }

}

// ------------------------------------------
// Standard Init/Handler 

void Mod_Timer::Init()
{
  Serial.println("modTimer_Init()");
  // lokalen Laufzeittimer initialisieren
  initRunTime();
  // sync vom ntp
  //mod_NTPClient.Init();
  syncFromNTP(); // gleich synchronisieen, sonst läuft der timer bei 0 los und synchonisiert erst in ein paar Stunden
}

void Mod_Timer::Handle()
{
	calcRunTime();
}

// ------------------------------------------

