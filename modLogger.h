// Lokaler Ringspeicher Logger

#pragma once

const byte logCode_InternalLogInit  = 1;
const String logMsg_InternalLogInit = "Log gestartet";

// ------------------------------------------
// Benutzerdefinierte Codes, Meldung muss in logMsg hinterlegt werden
const byte logCode_Startup          = 0;
const byte logCode_StartupDone      = 1;

const byte logCode_IOManMode        = 2;

const byte logCode_IOoff            = 3;
const byte logCode_IOcharge         = 4;
const byte logCode_IOdischarge      = 5;
const byte logCode_Measure          = 6;
const byte logCode_VBattGes         = 7;
const byte logCode_VBatt1           = 8;
const byte logCode_VBatt2           = 9;

const int logMsgCount = 10;   // Antahl de definierten Meldungen, nicht Loggröße. Mindestens Anzahl der Einträge (merkt der Compiler) kann aber auch mehr sein
const String logMsg[logMsgCount] = {
  "Controller Init...",
  "Controller Init abgeschlossen",

  "Manueller IO Modus aktiviert",
  
  "IO: Aus",
  "IO: Laden",
  "IO: Entladen",
  "Messen",
  "Batteriespannung (ges)",
  "Batteriespannung (Batt 1)",
  "Batteriespannung (Batt 2)"
};

// --------------------------------------------
struct localLogItem {
  String    timeStamp;
  byte      logCode;
  byte      logValue;
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
    void Add(String timestamp, byte code, byte value);
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

void Mod_Logger::Add(String timestamp, byte code, byte value) {
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
  String msgAll = "";
  if (localLogIndex >= 0) { 
    /*
    // dump wie geschrieben
    for (int iLog = 0; iLog <= localLogSize-1; iLog++) {
     msgAll += DumpItem(iLog);
     msgAll += "\r\n";
    }
    */

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
