/*
  Arduino IDE Project for ESP8266 Board
  This is an Solar Battery Controler
  by Peter Lorenz
  support@peter-ebe.de
  
  Libraries used: (installed from libraries manager)
  - ArduinoHttpClient
  - NTPClient
  - Adafruit ADS1X15
  - Adafruit BUSIO
  - Arduinojson

  Libraries used: (installed to libraries path)
  - tinyxml2 

  Libraries used: (included by default)
  - ESP8266WiFi
  - ESP8266WebServer
  - WiFiClient
  - WiFiUdp

  Removed
  - Arduino
  - Scheduler
  - Task

  --------------------------------------------------------------------
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at https://mozilla.org/MPL/2.0/.

  THE SOFTWARE IS PROVIDED "AS IS" AND WITHOUT WARRANTY

  Last maintainer: Peter Lorenz
  You find the code useful? Donate!
  Paypal webmaster@peter-ebe.de
  --------------------------------------------------------------------
  
*/

#include "modstaticWifi.h"
#include "modstaticWebInterface.h"

#include "modNTPClient.h"
#include "modTimer.h"
#include "modLogger.h"

#include "modIO.h"
#include "modEMeterClient.h"
#include "modBatteryWR.h"
#include "modPowermeter.h"

#include "modPowerControl.h"

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
  ModStatic_Wifi::init();

  // Webinterface
  ModStatic_WebInterface::init(); 

  // Timer
  mod_NTPClient.init();
  mod_Timer.init();  // benötigt Wifi für initiales ntp update

  // Logger 
  mod_Logger.init(); // sollte immer mit Zeitstempel vom Timer aufgerufen werden

  mod_Logger.add(mod_Timer.runTimeAsString(), logCode_Startup, 0); // Jetzt kommen alle anderen Module und Steuerungsmodul(e) -> 

  // EMeter Client
  mod_EMeterClient.init();

  // Battery WR Client
  mod_BatteryWRClient.init();

  // Powermeter
  mod_PowerMeter.init();

  // I/O
  mod_IO.init();

  // Jetzt der Controller

  // Controller
  mod_PowerControl.init();
  prg_Controller.init(); 

  mod_Logger.add(mod_Timer.runTimeAsString(), logCode_StartupDone, 0); // <- Fertig

  Serial.println("------------------------------------------------");
}

void loop() {
  // Achtung, nicht blockieren!
  // Loop wird auch bei Verwendung des Schedulers (sofern ich ihn verwende) bei jedem Durchlauf durchlaufen!

  // Netzwerk
  ModStatic_Wifi::handle();

  // WebInterface für Benutzerabfrage und Eingriffe
  ModStatic_WebInterface::handle();

  // Lokaler Timer, dieser muss immer zu Beginn der Loop aufgerufen werden
  //modNTPClient_handle(); // nicht nötig, von mod_Timer verwendet
  mod_Timer.handle();

  // EMeter Client
  //mod_EMeterClient.handle(); // nicht nötig, bei Bedarf

  // Battery WR Client
  //mod_BatteryWRClient.handle(); // nicht nötig, bei Bedarf

  // Powermeter
  mod_PowerMeter.handle();

  // IO 
  mod_IO.handle();

  // Logger
  //mod_Logger.handle(); // nicht nötig, bei Bedarf

  // Controller für die automatische Steuerung, dieser muss immer als leztes in der Loop aufgerufen werden
  //mod_PowerControl.handle(); // nicht nötig, bei Bedarf 
  prg_Controller.handle(); 

}
