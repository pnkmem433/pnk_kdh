#include "pir.h"

Pir::Pir(int pin) : pin(pin), state(false) {}

void Pir::begin() {
  pinMode(pin, INPUT);

  xTaskCreate(
      [](void *param) {
        Pir *pir = static_cast<Pir *>(param);

        bool isChanged = false;
        while (true) {
          bool state = digitalRead(pir->pin);
          if (state && isChanged == false) {
            isChanged = true;
            pir->state = true;
          } else
            isChanged = false;

          vTaskDelay(10 / portTICK_PERIOD_MS);
        }
      },
      "PIR Task", 2048, this, 1, nullptr);
}

void Pir::clear() { state = false; }

bool Pir::isDetected() { return state; }