// Lokaler Ringspeicher Logger

#pragma once

const byte logCode_InternalLogInit  = 1;
const String logMsg_InternalLogInit = "Log gestartet";

// ------------------------------------------
// Benutzerdefinierte Codes, Meldung muss hinterlegt im darauf folgenden Array werden
// (da diese Codes nur zur Laufzeit gültig sind und nicht permanent gespeichert werden, können sie nachträglich geändert werden solange das Array mit angepasst wird)
const byte logCode_Startup              = 0;
const byte logCode_StartupDone          = 1;

const byte logCode_IOoff                = 2;
const byte logCode_IOcharge             = 3;
const byte logCode_IOdischarge          = 4;

const byte logCode_Measure              = 5;
const byte logCode_VBattGes             = 6;
const byte logCode_VBattGesProz         = 7;
const byte logCode_VBatt1               = 8;
const byte logCode_VBatt2               = 9;
const byte logCode_PVPower              = 10;
const byte logCode_EMeterPower          = 11;

const byte logCode_IOmanIOModeOn        = 12;
const byte logCode_IOmanIOModeOff       = 13;

const byte logCode_IOmanBattSimuOn      = 14;
const byte logCode_IOmanBattSimuOff     = 15;
const byte logCode_Free1                = 16;

const byte logCode_SystemFailure        = 17;

const byte logCode_StartCharge          = 18;
const byte logCode_StartDischarge       = 19;
const byte logCode_StartChargeEmergency = 20;

const byte logCode_StopCharge           = 21;
const byte logCode_StopDischarge        = 22;
const byte logCode_StopChargeEmergency  = 23;

const byte logCode_PVSimuOn             = 24;
const byte logCode_PVSimuOff            = 25;
const byte logCode_PVRequestFail        = 26;

const byte logCode_EMeterSimuOn         = 27;
const byte logCode_EMeterSimuOff        = 28;
const byte logCode_EMeterRequestFail    = 29;

const byte logCode_TimeSimuOff          = 30;
const byte logCode_TimeSimuDay          = 31;
const byte logCode_TimeSimuNight        = 32;

const int logMsgCount = 33;   // Anzahl der hinterlegten Meldungen. Entspricht der Anzahl der Einträge (weniger merkt der Compiler, mehr nicht)
const String logMsg[logMsgCount] = {
  "<b><font color=green>Controller Init...</font></b>",
  "<b><font color=green>Controller Init abgeschlossen</font></b>",

  "IO: Aus",
  "IO: Laden",
  "IO: Entladen",

  "Messen",
  "Batteriespannung (ges)",
  "Batteriespannung (ges proz)",
  "Batteriespannung (Batt 1)",
  "Batteriespannung (Batt 2)",
  "PV Leistung",
  "Bezug(+) Einspeisung(-) Leistung",

  "<b>Manueller IO Modus aktiviert</b>",
  "<b>Menueller IO Modus deaktiviert</b>",  
  "<b>Batteriesimulation an</b>",
  "<b>Batteriesimulation aus</b>",
  "(free)",

  "<b><font color=red>Steurung auf Fehlerzustand</font></b>",

  "<b><font color=green>Ladevorgang gestartet</font></b>",
  "<b><font color=green>Entladevorgang gestartet</font></b>",
  "<b><font color=orange>Ladevorgang gestartet (Tiefentladeschutz)</font></b>",

  "<b><font color=green>Ladevorgang beendet</font></b>",
  "<b><font color=green>Entladevorgang beendet</font></b>",
  "<b><font color=orange>Ladevorgang beendet (Tiefentladeschutz)</font></b>",

  "<b>PVsimulation an</b>",
  "<b>PVsimulation aus</b>",
  "<b><font color=red>PV Abfrage fehlgeschlagen</font></b>",

  "<b>EMetersimulation an</b>",
  "<b>EMetersimulation aus</b>",
  "<b><font color=red>EMeter Abfrage fehlgeschlagen</font></b>",

  "<b>Zeitsimulation aus</b>",
  "<b>Zeitsimulation Tag</b>",
  "<b>Zeitsimulation Nacht</b>",

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
    
    void Prepare();
    String DumpItem(uint iLog);
  public:
    void Add(String timestamp, byte code, float value);
    String Dump();

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void Init();
    void Handle();
};
Mod_Logger mod_Logger;

// ------------------------------------------

void Mod_Logger::Prepare() {
  Serial.println("localLog_Perpare()");
  for (int iLog = 0; iLog <= localLogSize-1; iLog++) {
    localLog[iLog].timeStamp = "";
    localLog[iLog].logCode = 0;
    localLog[iLog].logValue = 0;
  }
}

// ------------------------------------------
// Addlog und LogDump für den externen Aufruf

void Mod_Logger::Add(String timestamp, byte code, float value) {
  localLogIndex += 1;
  if (localLogIndex > localLogSize-1) {
    localLogIndex = 0;
  }
  localLog[localLogIndex].timeStamp = timestamp;
  localLog[localLogIndex].logCode = code;
  localLog[localLogIndex].logValue = value;
}

String Mod_Logger::DumpItem(uint iLog) {
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

String Mod_Logger::Dump() {

  // dump wie geschrieben
  //String msgDbg = "";  
  //for (int iLog = 0; iLog <= localLogSize-1; iLog++) {
  //  msgDbg += DumpItem(iLog);
  //  msgDbg += "\r\n";
  //}
  //Serial.println(msgDbg);

  String msgAll = "";
  if (localLogIndex >= 0) { 
    // vorherige Logrunde,
    // vom aktuellen Index+1 bis Ende sofern der zuletzt geschriebener Index nicht der letzte war, also dahinter noch alte Einträge zu finden sind
    if (localLogIndex < localLogSize-1) {
      for (int iLog = localLogIndex+1; iLog <= localLogSize-1; iLog++) {
        msgAll += DumpItem(iLog);       
      }    
    }

    // vom Anfang bis zum aktuellen Index Loggen der zuletzt geschrieben wurde, das kann der erste aber auch der letzte sein
    for (int iLog = 0; iLog <= localLogIndex; iLog++) {
     msgAll += DumpItem(iLog);
    }
  }
  return msgAll;
}

// --------------------------------------------
// Standard Init/Handler 

void Mod_Logger::Init() {
  Serial.println("modLogger_Init()");

  Prepare();
  Add("00", logCode_InternalLogInit, 0); 

}

void Mod_Logger::Handle() {
  // der tut nichts
}

// --------------------------------------------
