// Lokaler Ringspeicher Logger

#pragma once

const byte logCode_InternalLogInit  = 1;
const String logMsg_InternalLogInit = "Log gestartet";

// ------------------------------------------
// Benutzerdefinierte Codes, Meldung muss hinterlegt im darauf folgenden Array werden
// (da diese Codes nur zur Laufzeit gültig sind und nicht permanent gespeichert werden, können sie nachträglich geändert werden solange das Array mit angepasst wird)
const byte logCode_None                 = 0;
const byte logCode_Separator            = 1;

const byte logCode_Startup              = 2;
const byte logCode_StartupDone          = 3;

const byte logCode_IOoff                = 4;
const byte logCode_IOcharge             = 5;
const byte logCode_IOdischarge          = 6;

const byte logCode_Measure              = 7;
const byte logCode_VBattActive          = 8;
const byte logCode_VBattProz            = 9;
const byte logCode_VBatt1               = 10;
const byte logCode_VBatt2               = 11;
const byte logCode_PVPower              = 12;
const byte logCode_EMeterPower          = 13;

const byte logCode_IOmanIOModeOn        = 14;
const byte logCode_IOmanIOModeOff       = 15;

const byte logCode_IOmanBattSimuOn      = 16;
const byte logCode_IOmanBattSimuOff     = 17;
const byte logCode_Free1                = 18;

const byte logCode_SystemFailure        = 19;

const byte logCode_StartCharge          = 20;
const byte logCode_StartDischarge       = 21;
const byte logCode_StartChargeEmergency = 22;

const byte logCode_StopCharge           = 23;
const byte logCode_StopDischarge        = 24;
const byte logCode_StopChargeEmergency  = 25;

const byte logCode_Resynch              = 26;

const byte logCode_PVSimuOn             = 27;
const byte logCode_PVSimuOff            = 28;
const byte logCode_PVRequestFail        = 29;

const byte logCode_EMeterSimuOn         = 30;
const byte logCode_EMeterSimuOff        = 31;
const byte logCode_EMeterRequestFail    = 32;

const byte logCode_TimeSimuOff          = 33;
const byte logCode_TimeSimuDay          = 34;
const byte logCode_TimeSimuNight        = 35;

const byte logCode_WifiErrorDetected    = 36;

const byte logCode_BatteryWRSimuOn      = 37;
const byte logCode_BatteryWRSimuOff     = 38;
const byte logCode_BatteryWRRequestFail = 39;
const byte logCode_BatteryWRPowerSet    = 40;
const byte logCode_BatteryWRPower       = 41;

const byte logCode_extADCMeasureFailed  = 42;
const byte logCode_extADCok             = 43;
const byte logCode_extADCfailed         = 44;

const byte logCode_PowerMeterSimuOn     = 45;
const byte logCode_PowerMeterSimuOff    = 46;
const byte logCode_PowerMeterPower      = 47;

const byte logCode_BattSelect           = 48;

// Anzahl der hinterlegten Meldungen (maxlogcode+2) wegen 0 basierend und dummyeintrag 
// Entspricht der Anzahl der Einträge (weniger merkt der Compiler, mehr nicht)
// obacht, wenn ein komma vergessen wird, wird's schräg", deshalb der Dummyeintrag hinten dran
const int logMsgCount = 50;   

const String logMsg[logMsgCount] = {
  "",
  "-",
  "<b><font color=green>Controller Init...</font></b>",
  "<b><font color=green>Controller Init abgeschlossen</font></b>",

  "IO: Aus",
  "IO: Laden",
  "IO: Entladen",

  "Messen",
  "Batteriespannung (aktive Batt)",
  "Batteriespannung (proz)",
  "Batteriespannung (Batt 1)",
  "Batteriespannung (Batt 2)",
  "PV Leistung",
  "EMeter Bezug(+) Einspeisung(-) Leistung",

  "<b>Manueller IO Modus aktiviert</b>",
  "<b>Menueller IO Modus deaktiviert</b>",  
  "<b>Batteriesimulation an</b>",
  "<b>Batteriesimulation aus</b>",
  "(free)",

  "<b><font color=red>Steurung auf Fehlerzustand</font></b><!--ERROR-->",

  "<b><font color=green>Ladevorgang gestartet</font></b>",
  "<b><font color=green>Entladevorgang gestartet</font></b>",
  "<b><font color=orange>Ladevorgang gestartet (Tiefentladeschutz)</font></b>",

  "<b><font color=green>Ladevorgang beendet</font></b>",
  "<b><font color=green>Entladevorgang beendet</font></b>",
  "<b><font color=orange>Ladevorgang beendet (Tiefentladeschutz)</font></b>",

  "<b><font color=green>Resynchronisation</font></b>",

  "<b>PVsimulation an</b>",
  "<b>PVsimulation aus</b>",
  "<b><font color=red>PV Abfrage fehlgeschlagen</font></b><!--ERROR-->",

  "<b>EMetersimulation an</b>",
  "<b>EMetersimulation aus</b>",
  "<b><font color=red>EMeter Abfrage fehlgeschlagen</font></b><!--ERROR-->",

  "<b>Zeitsimulation aus</b>",
  "<b>Zeitsimulation Tag</b>",
  "<b>Zeitsimulation Nacht</b>",

  "<b><font color=red>WiFi-Fehler erkannt</font></b><!--ERROR-->",

  "<b>WRsimulation an</b>",
  "<b>WRsimulation aus</b>",
  "<b><font color=red>WR Abfrage/Setzen fehlgeschlagen</font></b>",
  "WR Leistungsvorgabe",
  "WR Leistung",

  "<b><font color=red>Messung von externen ADC fehlgeschlagen</font></b><!--ERROR-->",

  "Initialisieren des externen ADC erfolgreich",
  "<b><font color=red>Fehler beim Initialisieren des externen ADC</font></b><!--ERROR-->",

  "<b>PowerMetersimulation an</b>",
  "<b>PowerMetersimulation aus</b>",
  "PowerMeter Leistung",

  "Batterieauswahl(aktive Batterie)",

  "DUMMYEINTRAG_DO_NOT_DELETE" // letzer eintrag ohne abschließendes komma, damit kann ich das nicht vergessen und sehe wenn ich mich beim Erweitern der Liste vertan habe

};

// --------------------------------------------
struct localLogItem {
  String    timeStamp;
  byte      logCode;
  float     logValue;
};
const uint localLogSize   =  100;     // LogGröße, die ältesten Einträge werden nach Erreichen der Loggröße Überschrieben

// --------------------------------------------
class Mod_Logger {
  private:
    // internes Logger Array + Initialisierung
    uint       localLogIndex  = -1;       // zuletzt geschriebener Eintrag

    localLogItem localLog[localLogSize];  // Log
  
    void prepare();
    String dumpItem(uint iLog);
  public:
    void add(String timestamp, byte code, float value);
    String dump();

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void init();
    void handle();
};
Mod_Logger mod_Logger;

// ------------------------------------------

void Mod_Logger::prepare() {
  Serial.println("localLog_Perpare()");
  for (int iLog = 0; iLog <= localLogSize-1; iLog++) {
    localLog[iLog].timeStamp = "";
    localLog[iLog].logCode = 0;
    localLog[iLog].logValue = 0;
  }
}

// ------------------------------------------
// Addlog und LogDump für den externen Aufruf

void Mod_Logger::add(String timestamp, byte code, float value) {
  localLogIndex += 1;
  if (localLogIndex > localLogSize-1) {
    localLogIndex = 0;
  }
  Serial.println(timestamp + ":" + String(code) + ":" + String(value));
  localLog[localLogIndex].timeStamp = timestamp;
  localLog[localLogIndex].logCode = code;
  localLog[localLogIndex].logValue = value;
}

String Mod_Logger::dumpItem(uint iLog) {
  String msg = "";
  if (localLog[iLog].logCode > 0) {  // logcode muss gesetzt sein, timestamp sollte immer gesetzt werden, value ist optional
    msg += localLog[iLog].timeStamp;
    msg += " : ";
    if (localLog[iLog].logCode <= logMsgCount-1) {
      msg += logMsg[localLog[iLog].logCode]; // vordefineirte Meldung zum Logcode
      if (localLog[iLog].logValue != 0) {    // ggf. Value anhängen
        msg += " : " + String(localLog[iLog].logValue);
      }
    }
    else {
      msg += "LogCode " + localLog[iLog].logCode;
    }
    msg += "\r\n";
  }
  return msg;
}

String Mod_Logger::dump() {

  // dump wie geschrieben
  //String msgDbg = "";  
  //for (int iLog = 0; iLog <= localLogSize-1; iLog++) {
  //  msgDbg += dumpItem(iLog);
  //  msgDbg += "\r\n";
  //}
  //Serial.println(msgDbg);

  String msgAll = "";
  if (localLogIndex >= 0) { 
    // vorherige Logrunde,
    // vom aktuellen Index+1 bis Ende sofern der zuletzt geschriebener Index nicht der letzte war, also dahinter noch alte Einträge zu finden sind
    if (localLogIndex < localLogSize-1) {
      for (int iLog = localLogIndex+1; iLog <= localLogSize-1; iLog++) {
        msgAll += dumpItem(iLog);       
      }    
    }

    // vom Anfang bis zum aktuellen Index Loggen der zuletzt geschrieben wurde, das kann der erste aber auch der letzte sein
    for (int iLog = 0; iLog <= localLogIndex; iLog++) {
     msgAll += dumpItem(iLog);
    }
  }
  return msgAll;
}

// --------------------------------------------
// Standard Init/Handler 

void Mod_Logger::init() {
  Serial.println("modLogger_init()");

  prepare();
  add("00", logCode_InternalLogInit, 0);  // damit beginnt das log mit einem separator. Unscheinbar aber damit weiß ich das log is initialisiert

}

void Mod_Logger::handle() {
  // der tut nichts
}

// --------------------------------------------
