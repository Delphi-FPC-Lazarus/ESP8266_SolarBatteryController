// Template

#pragma once

//#include <ESP8266WiFi.h>
//#include <WiFiClient.h>

//#include <HttpClient.h>
//#include <tinyxml2.h>
//#include <ArduinoJson.h>
//using namespace tinyxml2;

// --------------------------------------------
class Mod_XXClient {
  private:
    float manXXSimu;

    //WiFiClient wifi;
    //HttpClient httpclient = HttpClient(wifi, "192.168.1.xxx", 80);

  public:
    void manXXSimuOn(float value);
    void manXXSimuOff();

    // Abfragefunktion für den externen Zugriff
    float GetCurrentPower(bool dolog); 

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void Init();
    void Handle();
};
Mod_XXClient mod_XXClient;

void Mod_XXClient::manXXSimuOn(float value) {
  manXXSimu = value;
  mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_XXSimuOn, value);
}
void Mod_XXClient::manXXSimuOff() {
  if (manXXSimu > 0) {
    manXXSimu = -1;
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_XXSimuOff, 0);
  }
}

// ------------------------------------------
// Abfragefunktion für den externen Zugriff

float Mod_XXClient::GetCurrentPower(bool dolog) {
  Serial.println("modXXClient_GetCurrentPower()");

  return -1;
}

// ------------------------------------------
// Standard Init/Handler 

void Mod_XXClient::Init()
{
  Serial.println("modXXClient_Init()");
  //Serial.println(GetCurrentPower(true));
  manXXSimu = -1;
}

void Mod_XXClient::Handle()
{
	// der standard handler tut nix, wenn der in der Mainloop mit aufgerufen würde, wäre die Hölle los
  // Es wird eine Abfrage Funktion zur Verfügung gestellt
}

// ------------------------------------------
