// WiFi

#pragma once

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>

// Include define from external header file
#include "WLANZUGANGSDATEN.h"
const char* ssid = STASSID;
const char* password = STAPSK;
const char* hostname = STAHOSTNAME;

// --------------------------------------------
class ModStatic_Wifi {
  private:
  public: 
    static void startWifi();
    static void stopWifi();
    static bool CheckConnected();

    static void Init();
    static void Handle();
};

// --------------------------------------------

int WiFiErrorCount = 0;

// --------------------------------------------

void ModStatic_Wifi::startWifi() {
  Serial.println("modWifi_startWifi()");

  WiFiErrorCount = 0;

  // MDNS
  if (MDNS.begin(hostname)) {
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

bool ModStatic_Wifi::CheckConnected() {
    if (WiFi.isConnected() == true) {
      WiFiErrorCount = 0;
      return true;
    } 
    else {
      WiFiErrorCount += 1;
      Serial.println("WifiFehler " + String(WiFiErrorCount));

      if (WiFiErrorCount < 60 ) {
        return true;
      }
      else {
        return false;
      }
    }
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

