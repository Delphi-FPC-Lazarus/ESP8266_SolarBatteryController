// EMeter Abfrage vom VZLogger über http Client

#pragma once

#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// benötigt aduinohttpclient und arduinojson
#include <HttpClient.h>
#include <ArduinoJson.h>

// --------------------------------------------
class Mod_EMeterClient {
  private:
    float manEMeterSimu;

    WiFiClient wifi;
    HttpClient httpclient = HttpClient(wifi, "192.168.1.252", 8080);

  public:
    void manEMeterSimuOn(float value);
    void manEMeterSimuOff();

    // Abfragefunktion für den externen Zugriff
    float GetCurrentPower(bool dolog); 

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void Init();
    void Handle();
};
Mod_EMeterClient mod_EMeterClient;

void Mod_EMeterClient::manEMeterSimuOn(float value) {
  manEMeterSimu = value;
  mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_EMeterSimuOn, value);
}
void Mod_EMeterClient::manEMeterSimuOff() {
  if (manEMeterSimu != 0) {
    manEMeterSimu = 0;
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_EMeterSimuOff, 0);
  }
}

// ------------------------------------------
// Abfragefunktion für den externen Zugriff

float Mod_EMeterClient::GetCurrentPower(bool dolog) {
  Serial.println("modEMeterClient_GetCurrentPower()");

  delay(1); // Yield()
  Serial.println("http get");
  httpclient.get("/");
  Serial.println("http get done");
  delay(1); // Yield()

  // read the status code and body of the response
  int statusCode = httpclient.responseStatusCode();
  Serial.println("Responsecode:" + String(statusCode));
  if ( statusCode != 200 ) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_EMeterRequestFail, float(statusCode));
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

  //const char* generator = doc["generator"];
  //Serial.println(generator);
  JsonObject dataitem = doc["data"][0];
  //const char* uuid = dataitem["uuid"];
  //Serial.println(uuid);
  float value = dataitem["tuples"][0][1];

  if (manEMeterSimu != 0) {
    value = manEMeterSimu;
  }

  //Serial.print("value "); Serial.println(value);
  if (dolog == true) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_EMeterPower,value);
  }

  return value;
}

// ------------------------------------------
// Standard Init/Handler 

void Mod_EMeterClient::Init()
{
  Serial.println("modEMeterClient_Init()");
  Serial.println(GetCurrentPower(true));
  manEMeterSimu = 0;
}

void Mod_EMeterClient::Handle()
{
	// der standard handler tut nix, wenn der in der Mainloop mit aufgerufen würde, wäre die Hölle los
  // Es wird eine Abfrage Funktion zur Verfügung gestellt
}

// ------------------------------------------
