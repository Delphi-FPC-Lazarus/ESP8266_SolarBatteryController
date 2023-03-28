// WiFi

#pragma once

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>

#include "WLANZUGANGSDATEN.h"

// --------------------------------------------
class ModStatic_Wifi {
  private:
    static void startWifi();
    static void stopWifi();

  public: 
    static void Init();
    static void Handle();
};

// --------------------------------------------

const char* ssid = STASSID;
const char* password = STAPSK;

// Set your Static IP address
IPAddress local_IP(192, 168, 1, 241);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 1, 1);   //optional
IPAddress secondaryDNS(192, 168, 1, 1); //optional

// --------------------------------------------

void ModStatic_Wifi::startWifi() {
  Serial.println("modWifi_startWifi()");

  // MDNS
  if (MDNS.begin("espSBC")) {
    Serial.println("MDNS responder started");
  }

  WiFi.mode(WIFI_STA);
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // Connect
  WiFi.begin(ssid, password);
  Serial.println("Connecting Wifi");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void ModStatic_Wifi::stopWifi() {
  Serial.println("modWifi_stopWifi()");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

// --------------------------------------------
// Standard Init/Handler 

void ModStatic_Wifi::Init() {
  Serial.println("modWifi_Init()");
  startWifi();
}

void ModStatic_Wifi::Handle() {
    // zyklisch aufgerufen
    MDNS.update();
}

// --------------------------------------------

