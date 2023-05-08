// NTP Client zur Zeit Abfrage aus dem Web

#pragma once

#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// benötigt NTPClient
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HttpClient.h>

// --------------------------------------------
class Mod_NTPClient {
  private:
    WiFiUDP ntpUDP;
    NTPClient timeClient = NTPClient(ntpUDP, "pool.ntp.org");
  public:
    // für den externen Zugriff 

    // Stunden Minuten Sekunden des aktuellen Tages
    int NTPhour;
    int NTPminute;
    int NTPsecond;

    // Aktuelles Datum und Uhrzeit
    time_t NTPepochTime;
    String epochTimeToString(time_t epochTime);

    void Update();

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void Init();
    void Handle();
};
Mod_NTPClient mod_NTPClient;

// --------------------------------------------

String Mod_NTPClient::epochTimeToString(time_t epochTime) {

  struct tm *ptm = gmtime ((time_t *)&epochTime); 
  int currentDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;
  int currentYear = ptm->tm_year+1900;
  int currenHour = ptm->tm_hour;
  int currentMin = ptm->tm_min;
  int currentSec = ptm->tm_sec;

  String resultString = "";
  if (currentDay < 10)    {resultString += "0"+String(currentDay)+".";}   else {resultString += String(currentDay)+".";};
  if (currentMonth < 10)  {resultString += "0"+String(currentMonth)+".";} else {resultString += String(currentMonth)+".";};
  if (currentYear < 10)   {resultString += "0"+String(currentYear)+" ";}  else {resultString += String(currentYear)+" ";};

  if (currenHour < 10)    {resultString += "0"+String(currenHour)+":";}   else {resultString += String(currenHour)+":";};
  if (currentMin < 10)    {resultString += "0"+String(currentMin)+":";}   else {resultString += String(currentMin)+":";};
  if (currentSec < 10)    {resultString += "0"+String(currentSec);}       else {resultString += String(currentSec);};

  return resultString;
}

// --------------------------------------------
// für den externen Aufruf 
void Mod_NTPClient::Update() {
  Serial.println("NTP Update");
  timeClient.update();
  Serial.println(epochTimeToString(timeClient.getEpochTime()));
  Serial.println(timeClient.getSeconds());
  NTPhour       = timeClient.getHours();
  NTPminute     = timeClient.getMinutes();
  NTPsecond     = timeClient.getSeconds();

  NTPepochTime  = timeClient.getEpochTime();
  Serial.print ("NTP Update: "); Serial.println(epochTimeToString(NTPepochTime));
}

// --------------------------------------------
// Standard Init/Handler 

// Für Deutschland gilt: Winterzeit = UTC/GMT +1. Sommerzeit = UTC/GMT +2.
// GMT +1 = 3600
// GMT 0 = 0
// GMT -1 = -3600

long winterzeit=3600;
long sommerzeit=3600*2;

void Mod_NTPClient::Init() {
  Serial.println("modHttpNTP_Init()");
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  timeClient.setTimeOffset(winterzeit);
}

void Mod_NTPClient::Handle() {
  // der standard handler tut nix, da ich den NTP Client nur vom Timer aus verwende und der ruft den NTP Update auf wenn er es braucht
}

// --------------------------------------------