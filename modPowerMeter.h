// Powermeter (Leistungsmessmodul)

#pragma once

// Powermeter Zuordnung AnalogIn 
#define ANALOG_IN 0

struct powermetercalibrationvalue {
  int value;
  int power;
};

// --------------------------------------------
class Mod_PowerMeter {
  private:
    float manPowerMeterSimu;
    byte triggertime_bak;

    // Notiz: delay() < 1ms compiliert, tut aber nichts                                              
    // analogRead() ist träge, 0,1ms auf dem esp8266
    // Wechselspannungsmessung bzw. nur positive Halbwelle kann hier vom adc gemessen werden
    // 50hz = 20ms -> 200 Samples je Sinus 
    // Um dieses Signal zu messen muss die Anzahl der Samples mindest ein Sinus sein, oder genau ein vielfaches davon
    static const int sampleCount = 600;
    int values[sampleCount]; // iterieren über array über samplecount oder (sizeof(values) / sizeof(values[0]))
    void sampleValues();

    // Messufunktionen
    int getMeasurementMean();

    // Für die Messung von dynamischen Lasten muss über einen längeren Zeitraum gemittelt werden
    static const int avgCount = 25; //100;
    bool avgActive = false;
    int avgValue = 0;
    void doAvgMeasurement();

    // Skalierung zum eingeseetzten getMeasurement() und Modul TA12-100
    // für Akkusystemcontroller
    static const int calibrationoffset = 0; // für die Kalibrierung auf 0 setzen
    static const int calibrationvaluecount = 13;  // tabelle nach offsetkorrektur
    powermetercalibrationvalue calibrationvalues[calibrationvaluecount] = {
      {0,0},
      {5,5},
      {6,23},
      {8,52},
      {11,75},
      {15,102},
      {18,128},
      {21,153},
      {25,178},
      {29,205},
      {37,229},
      {43,238},
      {77,580}
    };
    int getPowerFromValue(int value);

  public:
    void manPowerMeterSimuOn(float value);
    void manPowerMeterSimuOff();

    // Abfragefunktion für den externen Zugriff
    float GetCurrentPower(bool dolog); 
    float lastPower;

    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void Init();
    void Handle();
};
Mod_PowerMeter mod_PowerMeter;

void Mod_PowerMeter::manPowerMeterSimuOn(float value) {
  manPowerMeterSimu = value;
  mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_PowerMeterSimuOn, value);
}
void Mod_PowerMeter::manPowerMeterSimuOff() {
  if (manPowerMeterSimu > 0) {
    manPowerMeterSimu = -1;
    mod_Logger.Add(mod_Timer.runTimeAsString(),logCode_PowerMeterSimuOff, 0);
  }
}

// ------------------------------------------
// Samplefunktion, über n Sinus (obere Halbwelle) sampeln
void Mod_PowerMeter::sampleValues() {
  //Serial.println("sampleValues()");
  // hintereinander weg samplen, ohne Unterbrechung
  for (int i = 0; i < sampleCount - 1 ; i++) 
  {
    values[i] = analogRead(ANALOG_IN);
  } 
}

// ------------------------------------------
// Messfunktion
int Mod_PowerMeter::getMeasurementMean() {
  //Serial.println("getMeasurementMean()");

  sampleValues();

  yield();
  ESP.wdtFeed();

  // ausgehend von einer nicht sinus förmigen stromaufnahme den ungefähren strom ermitteln, strom aufintegrieren und durch die samples teilen
  // (um genau zu sein müsste man jeden Stromwert mit der Spannung multiplizieren und diese Leistung über die Zeit aufintegrieren)
  int measurementValue = 0; 
  int measurementValueCount = 0;
  for (int i = 0; i < sampleCount-1; i++) 
  {
    measurementValue += values[i];
    measurementValueCount += 1;
  }
  measurementValue = measurementValue / measurementValueCount;
  return measurementValue;
}

// ------------------------------------------
// Für die Messung von dynamischen Lasten muss über einen längeren Zeitraum gemittelt werden
void Mod_PowerMeter::doAvgMeasurement() {
  Serial.println("doAvgMeasurement");
  if (avgActive == true) {
    return;
  }
  avgActive = true;

  // for (int i = 0; i < 3; i++) 
  //{
  //  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
  //  delay(100);
  //  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH 
  //  delay(100);
  //} 
  yield();    
  ESP.wdtFeed(); 

  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level

  avgValue = 0;
  for (int i = 0; i <= (avgCount-1); i++) 
  {
      avgValue += getMeasurementMean();

      yield();    
      ESP.wdtFeed(); 
  }
  Serial.println("done");
  Serial.println(avgCount);
  Serial.println(avgValue);
  avgValue = avgValue / avgCount;
  Serial.println(avgValue);

  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH  

  avgActive = false;
}

// ------------------------------------------
// Messwert zu Leistung

int Mod_PowerMeter::getPowerFromValue(int value) {
  // suche Eintrag <= Value und >= Value
  value = value - calibrationoffset;
  int ilow=-1;
  int ihigh=-1;
  for (int i = 0; i < calibrationvaluecount; i++) {
    if (calibrationvalues[i].value <= value) {
      ilow = i;
    }
    if ( (calibrationvalues[i].value >= value) && (ihigh < 0) ) {
      ihigh = i;
    }
  }
  if ( ilow == ihigh) {
    // genauer Treffer in der Tabelle, hier brauch nichts berechnet zu werden
    return calibrationvalues[ilow].power; 
  }
  if (ilow < 0) {
    // außerhalb Kalibrationsbereich (low), darf nicht vorkommen, Leistungswert für 0 Value muss angegeben sein
    return 0;
  }
  if (ihigh < 0) {
    // außerhalb Kalibrationsbreich (high), letztes oberes wertepar verwenden
    ilow = calibrationvaluecount-2;
    ihigh = calibrationvaluecount-1;
  }
  //Serial.println(String(ilow)+"/"+String(ihigh)); // Debug

  int deltavalue = calibrationvalues[ihigh].value - calibrationvalues[ilow].value;
  int deltapower = calibrationvalues[ihigh].power - calibrationvalues[ilow].power;
  float faktor = float(deltapower) / float(deltavalue);
  //Serial.println(String(faktor)); // Debug

  float result = float(calibrationvalues[ilow].power) + ( float(value - calibrationvalues[ilow].value) * faktor );
  //Serial.println(String(result)); // Debug 

  return int(result);
}

// ------------------------------------------
// Abfragefunktion für den externen Zugriff

float Mod_PowerMeter::GetCurrentPower(bool dolog) {
  Serial.println("modPowerMeter_GetCurrentPower()");

  float pwr = 0;

  if (manPowerMeterSimu > 0) {
    pwr = manPowerMeterSimu;
  } else {
    doAvgMeasurement();
    float value = avgValue;
    Serial.print("Powermeter Value: "); Serial.println(value); 
    pwr = getPowerFromValue(value);
    Serial.print("Powermeter Leistung: "); Serial.println(pwr); 
  }

  if (dolog == true) {
    mod_Logger.Add(mod_Timer.runTimeAsString(), logCode_PowerMeterPower,pwr);
  }
  
  lastPower = pwr;
  return pwr;
}

// ------------------------------------------
// Standard Init/Handler 

void Mod_PowerMeter::Init()
{
  Serial.println("modPowerMeter_Init()");
  //Serial.println(GetCurrentPower(true));
  manPowerMeterSimu = -1;
  triggertime_bak = mod_Timer.runTime.m;
  lastPower = 0;

}

void Mod_PowerMeter::Handle()
{
	// der standard handler tut nix, wenn der in der Mainloop mit aufgerufen würde, wäre die Hölle los
  // Es wird eine Abfrage Funktion zur Verfügung gestellt
  
  // zyklische Messung für Bereitstellung
  if ( (mod_Timer.runTime.m != triggertime_bak) ) {
    triggertime_bak = mod_Timer.runTime.m;
    GetCurrentPower(false);
  }

}

// ------------------------------------------
