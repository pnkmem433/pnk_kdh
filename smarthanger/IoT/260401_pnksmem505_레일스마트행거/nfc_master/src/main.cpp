#include <Arduino.h>
#include <I2CMaster.h>
#include <TaskRunner.h>

// 소프트웨어 생성자: 스캔 태스크 설정
TaskRunner masterTask({
  .name = "i2c_scan", // 태스크 이름
  .stackSize = 4096,    // 태스크 스택 크기(바이트)
  .priority = 1,        // 태스크 우선순위
  .core = -1,           // -1 = 코어 고정 안 함
});

// 하드웨어 생성자: 실제 핀/버스/주소 설정
I2CMaster i2cMaster({
  .slaves = {0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f}, // I2C 슬레이브 주소 목록
  .sda = D4,             // 마스터 I2C SDA 핀(GPIO 번호)
  .scl = D5,             // 마스터 I2C SCL 핀(GPIO 번호)
  .frequency = 100000,   // I2C 버스 주파수(Hz)
  .task = &masterTask,   // 스캔을 돌릴 TaskRunner
});

/* 
  setup()/loop()는 src/app.cpp에 정의되어 있으며
  PlatformIO/Arduino 빌드가 모든 .cpp를 함께 링크하여 자동으로 사용됩니다.
*/
