#ifndef DOOR_SENSOR_H
#define DOOR_SENSOR_H

#include <Arduino.h>
#include <vector>


enum class DoorEvent { Opened, Closed };

constexpr int NO_DELAY = 0;

struct DoorSensorEvent {
  DoorEvent event;
  int delayMs;

  std::function<void()> action;
};

struct DoorEventMessage {
  int sensorPin;
  DoorEvent event;
  uint32_t delayMs;
  std::function<void()> action;
};

class DoorSensor {
public:
  DoorSensor(int pin, uint8_t pinMode = INPUT_PULLUP);
  void begin(std::vector<DoorSensorEvent> events);

private:
  int pin;
  uint8_t pinModeValue;
  std::vector<DoorSensorEvent> events;

  static QueueHandle_t eventQueue;
  static void workerTask(void *param);
};

#endif