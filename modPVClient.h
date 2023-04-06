// Solar PV Abfrage über http Client

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
    WiFiClient wifi;
    HttpClient httpclient = HttpClient(wifi, "192.168.1.251", 80);

    XMLDocument doc;
  public:
    // Abfragefunktion für den externen Zugriff
    double GetCurrentPower();

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void Init();
    void Handle();
};
Mod_PVClient mod_PVClient;

// ------------------------------------------
// Abfragefunktion für den externen Zugriff

double Mod_PVClient::GetCurrentPower() {
  Serial.println("modPVClient_GetCurrentPower()");

  httpclient.get("/measurements.xml");

  // read the status code and body of the response
  int statusCode = httpclient.responseStatusCode();
  String response = httpclient.responseBody();

  //Serial.print("Status code: "); Serial.println(statusCode);
  //Serial.print("Response: ");  Serial.println(response);

  response.replace("<?xml version='1.0' encoding='UTF-8'?>", "");
  const char* responsexmlchar = response.c_str();  
	doc.Parse(responsexmlchar);

  XMLElement* rootElement = doc.RootElement(); 
  //Serial.println(rootElement->Name());
  XMLNode* deviceNode = rootElement->FirstChild();
  //Serial.println(deviceNode->ToElement()->Attribute("DateTime"));
  XMLNode* measurementsNode = deviceNode->FirstChild();

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
      double Value = ValueStr.toDouble();
      //Serial.println("");
      //Serial.println(Value);
      //Serial.println("");
      return Value;
    }

    measurementNode = measurementNode->NextSibling();
  }

  return -1;
}

// ------------------------------------------
// Standard Init/Handler 

void Mod_PVClient::Init()
{
  Serial.println("modPVClient_Init()");
  //Serial.println(GetCurrentPower());
}

void Mod_PVClient::Handle()
{
	// der standard handler tut nix, wenn der in der Mainloop mit aufgerufen würde, wäre die Hölle los
  // Es wird eine Abfrage Funktion zur Verfügung gestellt
}

// ------------------------------------------
