// WebServer für den Remotezugriff auf den Controller

#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include "modIO.h"
#include "modLogger.h"
#include "modTimer.h"
#include "modPVClient.h"

// --------------------------------------------
class ModStatic_WebInterface {
  private:

  public:
    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    static void Init();
    static void Handle();
};

ESP8266WebServer server(80);
bool DoESPreset = false;

// --------------------------------------------

void handleMenue() {
  /*
  String message;
  for (int i = 0; i < server.args(); i++) {
    message += "Arg" + (String)i + " –> ";
    message += server.argName(i) + ": ";
    message += server.arg(i) + "\r\n";
  }
  Serial.println(message);
  */
  for (int i = 0; i < server.args(); i++) {
    Serial.println("handleMenue()"+server.argName(i));

    if (server.argName(i) == "reset") {
      DoESPreset = true;
    }
    // Laden und Entladen
    if (server.argName(i) == "off") {
      mod_IO.SetmanIOModeOn();
      mod_IO.Off();
    }
    if (server.argName(i) == "charge") {
      mod_IO.SetmanIOModeOn();
      mod_IO.Charge();
    }
    if (server.argName(i) == "discharge") {
      mod_IO.SetmanIOModeOn();
      mod_IO.Discharge();
    }
    if (server.argName(i) == "auto") {
      mod_IO.Off();
      mod_IO.SetmanIOModeOff();
    }

    if (server.argName(i) == "simubattoff") {
      mod_IO.SetmanBattSimuOff();
    }
    if (server.argName(i) == "simubatta") {
      mod_IO.SetmanBattSimuOn(27.0);
    }
    if (server.argName(i) == "simubattb") {
      mod_IO.SetmanBattSimuOn(26.5);
    }
    if (server.argName(i) == "simubattc") {
      mod_IO.SetmanBattSimuOn(26);
    }
    if (server.argName(i) == "simubattd") {
      mod_IO.SetmanBattSimuOn(25.5);
    }

    if (server.argName(i) == "simupvoff") {
      mod_PVClient.manPVSimuOff();
    }
    if (server.argName(i) == "simupva") {
      mod_PVClient.manPVSimuOn(1);
    }
    if (server.argName(i) == "simupvb") {
      mod_PVClient.manPVSimuOn(500);
    }
    if (server.argName(i) == "simupvc") {
      mod_PVClient.manPVSimuOn(1000);
    }

    //if (server.argName(i) == "") {
    //}

    // Messen
    if (server.argName(i) == "measurebattges") {
      mod_IO.MeasureBattGes(true);
    }
    if (server.argName(i) == "measurebatt12") {
      mod_IO.MeasureBatt12(true);
    }
    if (server.argName(i) == "measurepv") {
      mod_PVClient.GetCurrentPower(true);
    }
 
  }


} 

String generateMenue() {
  String menu = "<a href='?'>Refresh</a><br>";
  menu += "<br>";
  
  menu += "<b>Manuelles Steuerungsmenue</b><br>";
  if ( (mod_IO.IsmanIOMode() == true) ) {
    menu += "Manuelle Steuerung ist aktiv!<br>";
  }
  menu += "<a href='?reset'>Reset / Controller Neu starten</a>&nbsp;&nbsp;&nbsp;";
  menu += "<br>";
  menu += "Modus:&nbsp;";
  menu += "<a href='?auto'>Automatikmodus</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?off'>Aus</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?charge'>Laden</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?discharge'>Entladen</a>&nbsp;&nbsp;&nbsp;";
  menu += "<br>";
  menu += "Batterie&nbsp;";
  menu += "<a href='?simubattoff'>Messwert</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simubatta'>27</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simubattb'>26.5</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simubattc'>26</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simubattd'>25.5</a>&nbsp;&nbsp;&nbsp;";
  menu += "<br>";
  menu += "PV&nbsp;";
  menu += "<a href='?simupvoff'>Messwert</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simupva'>1</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simupvb'>500</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simupvc'>1000</a>&nbsp;&nbsp;&nbsp;";
  menu += "<br>";
  menu += "Messen&nbsp;";
  menu += "<a href='?measurebattges'>Batt ges</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?measurebatt12'>Batt 1/2</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?measurepv'>PV</a>&nbsp;&nbsp;&nbsp;";

  menu += "<br>";

  return menu;
}


void handleRoot() {
  Serial.println("handleRoot()");
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level

  // zuerst die URL Parameter auswerten
  handleMenue();

  String message = "<html><body>";
  message += "<h1>esp8266 solar battery controller</h1><br>";

  message += "WiFi";
  message += String(ssid);
  message += "<br>";
  message += "IP: ";
  message += String(WiFi.localIP().toString()) + "<br>";
  message += "<br>";
  message += "Freier Heap: "+String(ESP.getFreeHeap())+" bytes<br>";
  message += "<hr><br>";
  message += "Lokale Zeit: " + mod_Timer.runTimeAsString() + "<br>";

  message += "<hr><br>";
  message += "Log:<br>";
  String logdump = mod_Logger.Dump();
  logdump.replace("\r\n", "<br>");
  message += logdump;

  message += "<hr><br>";
  message += "<br>";
  
  message += generateMenue();

  message += "</body></html>";

  server.send(200, "text/html", message);
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH

}

void handleDebug() {
  Serial.println("handleDebug()");
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level

  String message = "";

  server.send(200, "text/plain", message);
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
}

// --------------------------------------------

void ModStatic_WebInterface::Init() {
  Serial.println("prgWebinterface_Init()");

  DoESPreset = false;

  server.on("/", handleRoot);
  server.on("/debug", handleDebug);

  server.begin();
  Serial.println("HTTP server started");
}

void ModStatic_WebInterface::Handle() {
  // zyklisch aufgerufen
  server.handleClient();

  if (DoESPreset == true) {
    // warten, damit Client in jedem Falle noch die Antwort bekommt
    Serial.println("delay");
    delay(3000);
    // Controller neu starten
    Serial.println("ESP reset/restart");
    ESP.restart(); //ESP.reset(); // restart ist der saubere Neustart
  }
}

// --------------------------------------------

