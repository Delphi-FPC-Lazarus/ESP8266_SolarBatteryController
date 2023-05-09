// Solar PV Abfrage Stecagrid 2500 über http Client

#pragma once

#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// benötigt aduinohttpclient und tinyxml2
#include <HttpClient.h>
#include <tinyxml2.h>

using namespace tinyxml2;

// --------------------------------------------
class Mod_PVClient {
  private:
    float manPVSimu;

    WiFiClient wifi;
    HttpClient httpclient = HttpClient(wifi, "192.168.1.251", 80);

    XMLDocument doc;
  public:
    void manPVSimuOn(float value);
    void manPVSimuOff();

    // Abfragefunktion für den externen Zugriff
    float GetCurrentPower(bool dolog); 

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void Init();
    void Handle();
};
Mod_PVClient mod_PVClient;

void Mod_PVClient::manPVSimuOn(float value) {
  manPVSimu = value;
  mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_PVSimuOn, value);
}
void Mod_PVClient::manPVSimuOff() {
  if (manPVSimu > 0) {
    manPVSimu = -1;
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_PVSimuOff, 0);
  }
}

// ------------------------------------------
// Abfragefunktion für den externen Zugriff

float Mod_PVClient::GetCurrentPower(bool dolog) {
  Serial.println("modPVClient_GetCurrentPower()");

  delay(1); // Yield()
  Serial.println("http get");
  httpclient.get("/measurements.xml");
  Serial.println("http get done");
  delay(1); // Yield()

  // read the status code and body of the response
  int statusCode = httpclient.responseStatusCode();
  Serial.println("Responsecode:" + String(statusCode));
  if ( statusCode != 200 ) {
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_PVRequestFail, float(statusCode));
    return -1;
  }
  String response = httpclient.responseBody();

  //Serial.print("Status code: "); Serial.println(statusCode);
  //Serial.print("Response: ");  Serial.println(response);

  response.replace("<?xml version='1.0' encoding='UTF-8'?>", "");
  const char* responsexmlchar = response.c_str();  
	doc.Parse(responsexmlchar);

  delay(1); // Yield()

  // root
  XMLElement* rootElement = doc.RootElement(); 
  if (rootElement == NULL) { Serial.println("rootElement Null"); return -1; }
  //Serial.println(rootElement->Name());
  
  // device
  XMLNode* deviceNode = rootElement->FirstChild();
  if (deviceNode == NULL) { Serial.println("deviceNode Null"); return -1; }
  //Serial.println(deviceNode->ToElement()->Attribute("DateTime"));

  // measurements
  XMLNode* measurementsNode = deviceNode->FirstChild();
  if (measurementsNode == NULL) { Serial.println("measurementsNode Null"); return -1; }

  // Measurement Elemente
  XMLNode* measurementNode = measurementsNode->FirstChild();
  while (measurementNode != NULL) {

    const char* attrType = measurementNode->ToElement()->Attribute("Type");
    //Serial.print(attrType);
    //Serial.print("=");
    const char* attrValue = measurementNode->ToElement()->Attribute("Value");
    //Serial.println(attrValue);

    if (String(measurementNode->ToElement()->Attribute("Type")) == "AC_Power") {  // "AC_Power" abfrage, "AC_Frequency" "DC_Voltage" (zum testen der Funktion)
      const char* ValueChar = measurementNode->ToElement()->Attribute("Value");
      String ValueStr = String(ValueChar);
      ValueStr.replace("-nan", "0");
      ValueStr.replace("nan", "0");
      float Value = ValueStr.toFloat();

      if (manPVSimu > 0) {
        Value = manPVSimu;
      }
 
      //Serial.println("");
      Serial.println(Value);
      //Serial.println("");

      if (dolog == true) {
        mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_PVPower,Value);
      }

      return Value;
    }

    delay(1); // Yield()
    measurementNode = measurementNode->NextSibling();
  }

  return -1;
}

// ------------------------------------------
// Standard Init/Handler 

void Mod_PVClient::Init()
{
  Serial.println("modPVClient_Init()");
  //Serial.println(GetCurrentPower(true));
  manPVSimu = -1;
}

void Mod_PVClient::Handle()
{
	// der standard handler tut nix, wenn der in der Mainloop mit aufgerufen würde, wäre die Hölle los
  // Es wird eine Abfrage Funktion zur Verfügung gestellt
}

// ------------------------------------------
