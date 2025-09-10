// NTP Client zur Zeit Abfrage aus dem Web

#pragma once

#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// benötigt NTPClient
#include <NTPClient.h>
#include <WiFiUdp.h>

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

    void calcTimeOffset();
    void update();

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void init();
    void handle();
};
Mod_NTPClient mod_NTPClient;

// Für Deutschland gilt: Winterzeit = UTC/GMT +1. Sommerzeit = UTC/GMT +2.
// GMT +1 = 3600
// GMT 0 = 0
// GMT -1 = -3600

long winterzeit=3600;
long sommerzeit=3600*2;

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
void Mod_NTPClient::update() {
  Serial.println("NTP Update");
  timeClient.update();
  
  NTPhour       = timeClient.getHours();
  NTPminute     = timeClient.getMinutes();
  NTPsecond     = timeClient.getSeconds();

  NTPepochTime  = timeClient.getEpochTime();
  Serial.println(epochTimeToString(NTPepochTime));
}

void Mod_NTPClient::calcTimeOffset() {

  struct tm *ptm = gmtime ((time_t *)&NTPepochTime); 
  int currentDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;
  //int currentYear = ptm->tm_year+1900;
  //int currenHour = ptm->tm_hour;
  //int currentMin = ptm->tm_min;
  //int currentSec = ptm->tm_sec;

  // In Deutschland wird zweimal im Jahr die Zeit umgestellt. 
  // Am letzten Sonntag im März erfolgt die Zeitumstellung von MEZ (bzw. Winterzeit) auf Sommerzeit
  // und am letzten Sonntag im Oktober von Sommerzeit auf MEZ (bzw. Winterzeit).
  // Hier vereinfacht [APRIL-OKTOBER]
  if ( (currentMonth >= 4) && (currentMonth <= 10) ) {
    timeClient.setTimeOffset(sommerzeit);
    Serial.println("Sommerzeit");
  } else {
    timeClient.setTimeOffset(winterzeit);
    Serial.println("Winterzeit");
  }

}

// --------------------------------------------
// Standard Init/Handler 
void Mod_NTPClient::init() {
  Serial.println("modHttpNTP_init()");
  timeClient.begin();
  update();  // Datum holen
  calcTimeOffset(); // berechnen
  update();  // übernehmen
}

void Mod_NTPClient::handle() {
  // der standard handler tut nix, da ich den NTP Client nur vom Timer aus verwende und der ruft den NTP Update auf wenn er es braucht
}

// --------------------------------------------