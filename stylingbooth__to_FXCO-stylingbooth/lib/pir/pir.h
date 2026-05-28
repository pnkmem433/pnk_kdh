#ifndef PIR_H
#define PIR_H

#include <Arduino.h>

class Pir {
    private:
        int pin; // PIR 센서 핀 번호
        bool state; // PIR 센서 상태
    public:
    Pir(int pin);

    void begin();
    
    bool isDetected();
    void clear();

};

#endif