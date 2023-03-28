
#pragma once

// --------------------------------------------
class Prg_Controller {
  private:

  public:
    // Standard Funktionen für Setup und Loop Aufruf aus dem Hauptprogramm
    void Init();
    void Handle();
};
Prg_Controller prg_Controller;

// --------------------------------------------

void Prg_Controller::Init() {
  Serial.println("prgController_Init()");
  // Todo


}

void Prg_Controller::Handle() {
  // Todo

}

// --------------------------------------------