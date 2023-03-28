/*
  Arduino IDE Project for ESP8266 Board

  This is an Solar Battery Controler
  
  by Peter Lorenz
  support@peter-ebe.de
*/

/*
#include <Arduino.h>
#include <Scheduler.h>
#include <Task.h>
*/

#include "modstaticWifi.h"
#include "modstaticWebInterface.h"

#include "modNTPClient.h"
#include "modTimer.h"
#include "modPVClient.h"
#include "modIO.h"
#include "modLogger.h"

#include "prgController.h"

void setup() {
  // Setup
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH

  Serial.begin(115200);
  Serial.println("Serial Monitor");

  // Watchdog
  // Es gibt einen hardware und einen software watchdog
  ESP.wdtDisable();     // Softwarewatchdog aus  
  ESP.wdtEnable(5000);  // Softwarewatchdog auf n sek stellen, achtung wenn ich ihn abgeschaltet lasse löst aller 6-7 Sekunden der Hardware watchdog aus

  // Achtung, reihenfolge beachten

  // Wifi
  ModStatic_Wifi::Init();

  // Webinterface
  ModStatic_WebInterface::Init(); 

  // Timer
  mod_NTPClient.Init();
  mod_Timer.Init();  // benötigt Wifi für initiales ntp update

  // Logger 
  mod_Logger.Init(); // sollte immer mit Zeitstempel vom Timer aufgerufen werden

  mod_Logger.Add(mod_Timer.runTimeAsString(), logCode_Startup, 0); // Jetzt kommen alle anderen Module und Steuerungsmodul(e) -> 

  // PV Client
  mod_PVClient.Init();

  // I/O
  mod_IO.Init();

  // Jetzt der Controller

  // Controller
  prg_Controller.Init(); 

  mod_Logger.Add(mod_Timer.runTimeAsString(), logCode_StartupDone, 0); // <- Fertig
}

void loop() {
  // Achtung, nicht blockieren!
  // Loop wird auch bei Verwendung des Schedulers (sofern ich ihn verwende) bei jedem Durdchlauf durchlaufen!

  // Netzwerk
  ModStatic_Wifi::Handle();

  // WebInterface für Benutzerabfrage und Eingriffe
  ModStatic_WebInterface::Handle();

  // Lokaler Timer (verwendet NTP Client, der muss nicht separat aufgerufen werden)
  //modNTPClient_Handle(); // nicht nötig
  mod_Timer.Handle();

  // PVClient
  //mod_PVClient.Handle(); // nicht nötig

  // IO 
  //mod_IO.Handle(); // nicht nötig 

  // Logger
  //mod_Logger.Handle(); // nicht nötig

  // Controller für die automatische Steuerung
  prg_Controller.Handle();

}