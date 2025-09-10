// Abfrage und Set des Wechselrichters des Akkusystems

#pragma once

#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// benötigt aduinohttpclient und arduinojson
#include <HttpClient.h>
#include <ArduinoJson.h>

// --------------------------------------------
class Mod_BatteryWRClient {
  private:
    bool manBatteryWRSimu;
    float manBatteryWRSimuValue;

  public:
    void manBatteryWRSimuOn();
    void manBatteryWRSimuOff();

    // Abfragefunktion für den externen Zugriff
    float getCurrentPower(bool dolog); 
    bool setPowerLimit(float pwr);

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void init();
    void handle();
};
Mod_BatteryWRClient mod_BatteryWRClient;

void Mod_BatteryWRClient::manBatteryWRSimuOn() {
  manBatteryWRSimu = true;
  manBatteryWRSimuValue = 0;
  mod_Logger.add(mod_Timer.runTimeAsString(),logCode_BatteryWRSimuOn, manBatteryWRSimuValue);
}
void Mod_BatteryWRClient::manBatteryWRSimuOff() {
  if (manBatteryWRSimu == true) {
    manBatteryWRSimu = false;
    manBatteryWRSimuValue = 0;
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_BatteryWRSimuOff, 0);
  }
}

// ------------------------------------------
// Abfragefunktion für den externen Zugriff

float Mod_BatteryWRClient::getCurrentPower(bool dolog) {
  Serial.println("modBatteryWRClient_getCurrentPower()");

  WiFiClient wifi;
  HttpClient httpclient = HttpClient(wifi, BATTERYWRIP, BATTERYWRPORT);

  delay(100); // Yield()

  if (!httpclient.connected()) {
    Serial.println("http not connected"); 
    // client macht einen reconnect beim request
  }

  Serial.println("http get");
  httpclient.get("/api/inverter/id/0");
  Serial.println("http get done");

  delay(100); // Yield()

  // read the status code and body of the response
  int statusCode = httpclient.responseStatusCode();
  Serial.println("Responsecode:" + String(statusCode));
  if ( statusCode != 200 ) {
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_BatteryWRRequestFail, float(statusCode));
    return 0;
  }
  String response = httpclient.responseBody();

  //Serial.print("Status code: "); Serial.println(statusCode);
  //Serial.print("Response: ");  Serial.println(response);

  const char* responsejsonchar = response.c_str();  
	
  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, responsejsonchar);
  if (err != DeserializationError::Ok) { Serial.print("DeserializationError "); Serial.println(err.f_str()); return 0; }
  if (doc == NULL) { Serial.println("doc Null"); return 0; }

  delay(1); // Yield()

  float value = doc["ch"][0][2];

  if (manBatteryWRSimu == true) {
    value = manBatteryWRSimuValue;
  }

  //Serial.print("value "); Serial.println(value);
  if (dolog == true) {
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_BatteryWRPower,value);
  }

  return value;
}

bool Mod_BatteryWRClient::setPowerLimit(float pwr) {
  Serial.println("modBatteryWRClient_SetLimit("+String(pwr)+")");

  if (manBatteryWRSimu == true) {
    Serial.println("Mod_BatteryWRClient::setPowerLimit skip");
    manBatteryWRSimuValue = pwr; 
    return true;
  }

  // mod_Logger.add(mod_Timer.runTimeAsString(),logCode_BatteryWRPowerSet,pwr); passiert im dischargemodus ständig, deshalb nicht ins log

  WiFiClient wifi;
  HttpClient httpclient = HttpClient(wifi, BATTERYWRIP, BATTERYWRPORT);

  delay(100); // Yield()

  Serial.println("http post");
  httpclient.post("/api/ctrl", "application/json", "{ \"id\": 0, \"cmd\": \"limit_nonpersistent_absolute\", \"val\": "+String(pwr)+"}");
  Serial.println("http post done");

  delay(100); // Yield()

  // read the status code and body of the response
  int statusCode = httpclient.responseStatusCode();
  Serial.println("Responsecode:" + String(statusCode));
  if ( statusCode != 200 ) {
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_BatteryWRRequestFail, float(statusCode));
    return 0;
  }
  String response = httpclient.responseBody();

  Serial.print("Status code: "); Serial.println(statusCode);
  Serial.print("Response: ");  Serial.println(response);

  const char* responsejsonchar = response.c_str();  
	
  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, responsejsonchar);
  if (err != DeserializationError::Ok) { Serial.print("DeserializationError "); Serial.println(err.f_str()); return 0; }
  if (doc == NULL) { Serial.println("doc Null"); return 0; }

  delay(1); // Yield()

  bool value = doc["success"];

  return value;
}

// ------------------------------------------
// Standard Init/Handler 

void Mod_BatteryWRClient::init()
{
  Serial.println("modBatteryWRClient_init()");
  Serial.println(getCurrentPower(true));
  manBatteryWRSimu = false;
  manBatteryWRSimuValue = 0;
}

void Mod_BatteryWRClient::handle()
{
	// der standard handler tut nix, wenn der in der Mainloop mit aufgerufen würde, wäre die Hölle los
  // Es wird eine Abfrage Funktion zur Verfügung gestellt
}

// ------------------------------------------
