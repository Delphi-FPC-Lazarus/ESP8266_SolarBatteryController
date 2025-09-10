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

  public:
    void manEMeterSimuOn(float value);
    void manEMeterSimuOff();

    // Abfragefunktion für den externen Zugriff
    float getCurrentPower(bool dolog); 

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void init();
    void handle();
};
Mod_EMeterClient mod_EMeterClient;

void Mod_EMeterClient::manEMeterSimuOn(float value) {
  manEMeterSimu = value;
  mod_Logger.add(mod_Timer.runTimeAsString(),logCode_EMeterSimuOn, value);
}
void Mod_EMeterClient::manEMeterSimuOff() {
  if (manEMeterSimu != 0) {
    manEMeterSimu = 0;
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_EMeterSimuOff, 0);
  }
}

// ------------------------------------------
// Abfragefunktion für den externen Zugriff

float Mod_EMeterClient::getCurrentPower(bool dolog) {
  Serial.println("modEMeterClient_getCurrentPower()");

  WiFiClient wifi;
  HttpClient httpclient = HttpClient(wifi, EMETERIP, EMETERPORT);

  delay(100); // Yield()

  if (!httpclient.connected()) {
    Serial.println("http not connected"); 
    // client macht einen reconnect beim request
  }

  Serial.println("http get");
  httpclient.get("/");
  Serial.println("http get done");

  delay(100); // Yield()

  // read the status code and body of the response
  int statusCode = httpclient.responseStatusCode();
  Serial.println("Responsecode:" + String(statusCode));
  if ( statusCode != 200 ) {
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_EMeterRequestFail, float(statusCode));
    return 0;
  }
  String response = httpclient.responseBody();

  //Serial.print("Status code: "); Serial.println(statusCode);
  //Serial.print("Response: ");  Serial.println(response);

  const char* responsejsonchar = response.c_str();  
	
  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, responsejsonchar);
  if (err != DeserializationError::Ok) { Serial.println("DeserializationError "); Serial.println(err.f_str()); return 0; }
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
    mod_Logger.add(mod_Timer.runTimeAsString(),logCode_EMeterPower,value);
  }

  return value;
}

// ------------------------------------------
// Standard Init/Handler 

void Mod_EMeterClient::init()
{
  Serial.println("modEMeterClient_init()");
  Serial.println(getCurrentPower(true));
  manEMeterSimu = 0;
}

void Mod_EMeterClient::handle()
{
	// der standard handler tut nix, wenn der in der Mainloop mit aufgerufen würde, wäre die Hölle los
  // Es wird eine Abfrage Funktion zur Verfügung gestellt
}

// ------------------------------------------
