// WebServer für den Remotezugriff auf den Controller

#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>

// Die folgenden Includes sind für das ganze Projekt gültig
#include "modLogger.h"
#include "modTimer.h"

#include "modIO.h"
#include "modEMeterClient.h"
#include "modBatteryWR.h"
#include "modPowerMeter.h"

#include "modPowerControl.h"

#include "prgController.h"

// --------------------------------------------
class ModStatic_WebInterface {
  private:

  public:
    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    static void init();
    static void handle();
};

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer updateserver;
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

    // Controller Neustart
    if (server.argName(i) == "reset") {
      DoESPreset = true;
    }

    // Messen
    if (server.argName(i) == "measureBattActive") {
      mod_IO.measureBattActive(true);
    }
    if (server.argName(i) == "measureBatt12") {
      mod_IO.measureBatt12(true);
    }
    if (server.argName(i) == "measureemeter") {
      mod_EMeterClient.getCurrentPower(true);
    }
    if (server.argName(i) == "measurewr") {
      mod_BatteryWRClient.getCurrentPower(true);
    }
    if (server.argName(i) == "measurepowermeter") {
      mod_PowerMeter.getCurrentPower(true);
    }

    // Manuelle Steuerung zum Laden und Entladen
    if (server.argName(i) == "off") {
      prg_Controller.setManualModeOn();
    }
    if (server.argName(i) == "charge") {
      prg_Controller.setManualModeOn();
      mod_IO.setCharge();
    }
    if (server.argName(i) == "discharge") {
      prg_Controller.setManualModeOn();
      mod_IO.setDischarge();
    }
    if (server.argName(i) == "auto") {
      prg_Controller.setManualModeOff();
    }

    // Batterie Simulation
    if (server.argName(i) == "simubatt1select") {
      if (mod_IO.isOff()) {
        mod_IO.selectBattActive(1);
      }
    } 
    if (server.argName(i) == "simubatt2select") {
      if (mod_IO.isOff()) {
        mod_IO.selectBattActive(2);
      }

    } 
    if (server.argName(i) == "simubatt1off") {
      mod_IO.setManBattSimuOff(1);
    }
    if (server.argName(i) == "simubatt1a") {
      mod_IO.setManBattSimuOn(1, 27.0);
    }
    if (server.argName(i) == "simubatt1b") {
      mod_IO.setManBattSimuOn(1, 26.5);
    }
    if (server.argName(i) == "simubatt1c") {
      mod_IO.setManBattSimuOn(1, 26);
    }
    if (server.argName(i) == "simubatt1d") {
      mod_IO.setManBattSimuOn(1, 25.7);
    }
    if (server.argName(i) == "simubatt1e") {
      mod_IO.setManBattSimuOn(1, 25.5);
    }
  
    if (server.argName(i) == "simubatt2off") {
      mod_IO.setManBattSimuOff(2);
    }
    if (server.argName(i) == "simubatt2a") {
      mod_IO.setManBattSimuOn(2, 27.0);
    }
    if (server.argName(i) == "simubatt2b") {
      mod_IO.setManBattSimuOn(2, 26.5);
    }
    if (server.argName(i) == "simubatt2c") {
      mod_IO.setManBattSimuOn(2, 26);
    }
    if (server.argName(i) == "simubatt2d") {
      mod_IO.setManBattSimuOn(2, 25.7);
    }
    if (server.argName(i) == "simubatt2e") {
      mod_IO.setManBattSimuOn(2, 25.5);
    }

    // Emeter Simulation
    if (server.argName(i) == "simuemeteroff") {
      mod_EMeterClient.manEMeterSimuOff();
    }
    if (server.argName(i) == "simuemetera") {
      mod_EMeterClient.manEMeterSimuOn(-600);
    }
    if (server.argName(i) == "simuemeterb") {
      mod_EMeterClient.manEMeterSimuOn(-300);
    }
    if (server.argName(i) == "simuemeterc") {
      mod_EMeterClient.manEMeterSimuOn(1);
    }
    if (server.argName(i) == "simuemeterd") {
      mod_EMeterClient.manEMeterSimuOn(200);
    }
    if (server.argName(i) == "simuemetere") {
      mod_EMeterClient.manEMeterSimuOn(1000);
    }

    // Wechselrichter Simulation
    if (server.argName(i) == "simuwroff") {        
      mod_BatteryWRClient.manBatteryWRSimuOff();
    }
    if (server.argName(i) == "simuwra") {
      mod_BatteryWRClient.manBatteryWRSimuOn();
    }
    if (server.argName(i) == "simuwrena") {
      mod_BatteryWRClient.setEnable();
    }    
    if (server.argName(i) == "simuwrdisa") {
      mod_BatteryWRClient.setDisable();
    }
    // PowerMeter Simulation
    if (server.argName(i) == "simuPowerMeteroff") {
      mod_PowerMeter.manPowerMeterSimuOff();
    }
    if (server.argName(i) == "simuPowerMetera") {
      mod_PowerMeter.manPowerMeterSimuOn(1);
    }
    if (server.argName(i) == "simuPowerMeterb") {
      mod_PowerMeter.manPowerMeterSimuOn(250);
    }
    if (server.argName(i) == "simuPowerMeterc") {
      mod_PowerMeter.manPowerMeterSimuOn(500);
    }

    // Zeitsteuerung Simulation (log in dem fall erst nach dem setzen)
    if (server.argName(i) == "simutimeoff") {
        mod_Timer.syncFromNTP();
        mod_Logger.add(mod_Timer.runTimeAsString(),logCode_TimeSimuOff,0);
    }
    if (server.argName(i) == "simutimeday") {
        mod_Timer.runTime.h = 12;
        mod_Timer.runTime.m = 0;
        mod_Timer.runTime.s = 0;
        mod_Logger.add(mod_Timer.runTimeAsString(),logCode_TimeSimuDay,0);
    }
    if (server.argName(i) == "simutimenight") {
        mod_Timer.runTime.h = 2;
        mod_Timer.runTime.m = 0;
        mod_Timer.runTime.s = 0;
        mod_Logger.add(mod_Timer.runTimeAsString(),logCode_TimeSimuNight,0);
    }
    if (server.argName(i) == "simutimeinchour") {
        if (mod_Timer.runTime.h < 23) {
          mod_Timer.runTime.h += 1;
          mod_Timer.runTime.m = 57;
        } else {
          mod_Timer.runTime.d += 1;
          mod_Timer.runTime.h = 0;
          mod_Timer.runTime.m = 57;
        }
    }
    if (server.argName(i) == "simutimeincday") {
        mod_Timer.runTime.d += 1;
        mod_Timer.runTime.h = 0;
        mod_Timer.runTime.m = 57;
    }

    //if (server.argName(i) == "") {
    //}

  }


} 

String generateMenue() {

  // Menü mit Links
  String menu = "<br>";
  menu += "<b>Messen</b><br>";
  menu += "<a href='?measureBattActive'>Batt aktiv</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?measureBatt12'>Batt 1/2</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?measureemeter'>E-Meter (Stromz&auml;hler)</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?measurewr'>Wechselrichter (Einspeisung)</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?measurepowermeter'>Powermeter (Laden/Einspeisen)</a>&nbsp;&nbsp;&nbsp;";
  menu += "<br><br>";
  menu += "<b>Fehlerbehebung</b><br>";
  menu += "<a href='?reset'>Reset / Controller Neu starten</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='/update'>Firmwareupdate</a>&nbsp;&nbsp;&nbsp;";

  menu += "<br><br><hr><b>Nur f&uuml;r Entwicklung/Tests:</b><hr>";
  menu += "<br><b>Manuelles Steuerungsmenue</b><br>";

  menu += "Modus:&nbsp;";
  menu += "<a href='?auto'>Auto</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?off'>Aus</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?charge'>Laden</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?discharge'>Entladen</a>&nbsp;&nbsp;&nbsp;";
  if ( (mod_IO.isManIOMode() == true) ) {
    menu += "&nbsp;&nbsp;&nbsp; (Manuelle Steuerung ist aktiv!)";
  }  
  menu += "<br><br>";
  menu += "<b>Simulationsmenue</b><br>";
  menu += "Batterieauswahl&nbsp;";
  menu += "<a href='?simubatt1select'>Batterie 1</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simubatt2select'>Batterie 2</a>&nbsp;&nbsp;&nbsp;";
  menu += "<br>";
  menu += "Batterie1&nbsp;";
  menu += "<a href='?simubatt1off'>Auto</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simubatt1a'>27</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simubatt1b'>26.5</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simubatt1c'>26</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simubatt1d'>25.7</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simubatt1e'>25.5</a>&nbsp;&nbsp;&nbsp;";
  menu += "<br>";
  menu += "Batterie2&nbsp;";
  menu += "<a href='?simubatt2off'>Auto</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simubatt2a'>27</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simubatt2b'>26.5</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simubatt2c'>26</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simubatt2d'>25.7</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simubatt2e'>25.5</a>&nbsp;&nbsp;&nbsp;";
  menu += "<br>";
  menu += "Emeter&nbsp;";
  menu += "<a href='?simuemeteroff'>Auto</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simuemetera'>-600</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simuemeterb'>-300</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simuemeterc'>1</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simuemeterd'>200</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simuemetere'>1000</a>&nbsp;&nbsp;&nbsp;";
  menu += "<br>";
  menu += "WR&nbsp;";
  menu += "<a href='?simuwroff'>Auto</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simuwra'>Simulation</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simuwrena'>Einschalten</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simuwrdisa'>Ausschalten</a>&nbsp;&nbsp;&nbsp;";
  menu += "<br>";
  menu += "PowerMeter&nbsp;";
  menu += "<a href='?simuPowerMeteroff'>Auto</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simuPowerMetera'>1</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simuPowerMeterb'>250</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simuPowerMeterc'>550</a>&nbsp;&nbsp;&nbsp;";
  menu += "<br>";
  menu += "Zeit&nbsp;";
  menu += "<a href='?simutimeoff'>Auto</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simutimeinchour'>N&auml;chsteStunde</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simutimeincday'>N&auml;chsterTag</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simutimeday'>Tag</a>&nbsp;&nbsp;&nbsp;";
  menu += "<a href='?simutimenight'>Nacht</a>&nbsp;&nbsp;&nbsp;";

  menu += "<br><br><br>";

  return menu;
}


void handleRoot() {
  Serial.println("handleRoot()");
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level

  // zuerst die URL Parameter auswerten
  handleMenue();

  String message = "<html><head><meta http-equiv=\"refresh\" content=\"60; URL=?\"></head><body>";

  message += "<h1>esp8266 solar battery controller</h1>";
  message += "Softwareversion: ";
  message += SOFTWARE_VERSION;
  message += "<br>";

  // WiFi und Speicher
  message += "WiFi";
  message += String(ssid);
  message += "<br>";
  message += "IP: ";
  message += String(WiFi.localIP().toString()) + "<br>";
  message += "Freier Heap: "+String(ESP.getFreeHeap())+" bytes<br>";
  
  // Systemstatus 
  message += "<hr>";
  message += "<b>Zeit:</b>&nbsp;" + mod_Timer.runTimeAsString() + "(Tage hh:nn)";
  message += "&nbsp;&nbsp;&nbsp;&nbsp;";
  if ( (mod_IO.isManIOMode() == true) ) {
    message += "<b>Status:</b>&nbsp;Manueller Modus";
  }
  else {
    message += "<b>Status:</b>&nbsp;" + prg_Controller.getStateString();
  }
  
  if ( ( prg_Controller.getState() != "C") && ( prg_Controller.getState() != "D") )
  {
    // Alle Zustände außer ladung und entladung, von Fehler über Standby bis Bereitschaft
    message += "&nbsp;&nbsp;&nbsp;&nbsp;";
    mod_IO.measureBatt12(false);
    message += "<b>Akku1:";
    if (mod_IO.getBattActive() == 1) { message += "(aktiv)"; }
    message += "</b>&nbsp;"+String(mod_IO.vBatt_1proz)+"% ("+String(mod_IO.vBatt_1)+"V)";
    message += " / ";
    message += "<b>Akku2:";
    if (mod_IO.getBattActive() == 2) { message += "(aktiv)"; }
    message += "</b>&nbsp;"+String(mod_IO.vBatt_2proz)+"% ("+String(mod_IO.vBatt_2)+"V)";

    if (prg_Controller.getState() == "R") {
      message += "<br>";
      message += prg_Controller.getDetailsMsg();
    }
  }
  else {
    // Lade- oder Entadezustand
    message += "&nbsp;&nbsp;&nbsp;&nbsp;";
    if (mod_IO.getBattActive() == 1) { message += "Akku1:(aktiv)"; }
    if (mod_IO.getBattActive() == 2) { message += "Akku2:(aktiv)"; }

    if (prg_Controller.getState() == "D") {
      message += "<br>";
      message += prg_Controller.getDetailsMsg();
    }
  }
  message += "<hr>";
  message += "<a href='?'>Refresh</a><br>";
  
  // Log
  message += "<br>Log:<br>";
  String logdump = mod_Logger.dump();
  logdump.replace("\r\n", "<br>");
  message += logdump;

  message += "<a href='?'>Refresh</a><br>";
  
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

void handleState() {
  Serial.println("handleState()");
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level

  String message = prg_Controller.getState();

  server.send(200, "text/plain", message);
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
}

void handleStateString() {
  Serial.println("handleStateString()");
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level

  String message = prg_Controller.getStateString();

  server.send(200, "text/plain", message);
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
}

void handleStatePower() {
  Serial.println("handleStatePower()");
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level

  // Hier ist es besser im Modus Einspeisung die Leistung des Wechselrichters zu verwenden, die ist genauer
  // ansonsten das powermeter beim laden
  String message = "";
  if (prg_Controller.getState() == "D") {
    float currentWRpwr = mod_BatteryWRClient.getCurrentPower(false);
    message =  String(currentWRpwr);
  } else {
    mod_PowerMeter.getCurrentPower(false);
    message =  String(mod_PowerMeter.lastPower);
  }

  server.send(200, "text/plain", message);
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
}

// --------------------------------------------

void ModStatic_WebInterface::init() {
  Serial.println("prgWebinterface_init()");

  DoESPreset = false;

  server.on("/", handleRoot);
  server.on("/state", handleState);
  server.on("/statestr", handleStateString);
  server.on("/power", handleStatePower);

  server.on("/debug", handleDebug);

  updateserver.setup(&server);

  server.begin();

  Serial.println("HTTP server started");
}

void ModStatic_WebInterface::handle() {
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

