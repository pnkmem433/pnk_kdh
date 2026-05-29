#include <Arduino.h>
#include <I2CSlave.h>
#include <TaskRunner.h>
#include <nfc.h>

// 소프트웨어 생성자: NFC 감시용 태스크
TaskRunner nfcTask({
  .name = "nfc_loop", // RTOS 작업 이름
  .stackSize = 8192,   // 작업 스택 크기(바이트)
  .priority = 1,       // 작업 우선순위
  .core = -1,          // -1 = 코어 고정 안 함
});

// 소프트웨어 생성자: 두번째 NFC 감시용 태스크
TaskRunner nfcTask2({
  .name = "nfc_loop2", // RTOS 작업 이름
  .stackSize = 8192,   // 작업 스택 크기(바이트)
  .priority = 1,       // 작업 우선순위
  .core = -1,          // -1 = 코어 고정 안 함
});

// 소프트웨어 생성자: 에코 표시용 태스크
TaskRunner echoTask({
  .name = "echo_loop", // RTOS 작업 이름
  .stackSize = 8192,    // 작업 스택 크기(바이트)
  .priority = 1,        // 작업 우선순위
  .core = -1,           // -1 = 코어 고정 안 함
});

// 소프트웨어 생성자: NFC 교대 스캔 태스크
TaskRunner muxTask({
  .name = "nfc_mux", // RTOS 작업 이름
  .stackSize = 4096, // 작업 스택 크기(바이트)
  .priority = 1,     // 작업 우선순위
  .core = -1,        // -1 = 코어 고정 안 함
});


// 하드웨어 생성자: I2C 슬레이브 버스 설정
I2CSlave i2cSlave({
  .address = 0x0d,   // 이 슬레이브의 I2C 주소
  .sda = D6,         // 슬레이브 I2C SDA 핀(GPIO 번호) - XIAO ESP32C3 D4
  .scl = D7,         // 슬레이브 I2C SCL 핀(GPIO 번호) - XIAO ESP32C3 D5
  .frequency = 100000, // I2C 버스 주파수(Hz)
  .task = &echoTask, // 에코 수신을 돌릴 TaskRunner
  .queueMax = 16,     // 전송 큐 최대 개수(0이면 기본값)
});

// 하드웨어 생성자: NFC(RC522) 핀 설정
NfcReader nfcReader({
  .pin = {
    .SCK = D8,   // RC522 SCK 핀(GPIO 번호) - XIAO ESP32C3 D8
    .MISO = D9,  // RC522 MISO 핀(GPIO 번호) - XIAO ESP32C3 D9
    .MOSI = D10, // RC522 MOSI 핀(GPIO 번호) - XIAO ESP32C3 D10
    .SS = D3,    // RC522 SS/SDA 핀(GPIO 번호) - XIAO ESP32C3 D3
    .RST = D2,   // RC522 RST 핀(GPIO 번호) - XIAO ESP32C3 D1
    .IRQ = -1,  // RC522 IRQ 핀(사용 안 함)
  },
  .task = &nfcTask, // NFC 스캔을 돌릴 TaskRunner
  .settings = {
    .miss = 500,          // 카드 제거로 판단할 시간(ms)
    .irqMode = false,     // IRQ 모드 사용 여부
    .pollIntervalMs = 25, // 폴링 주기(ms)
    .reinitMisses = 8,    // 실패 누적 시 재초기화 횟수
  },
});


// 하드웨어 생성자: NFC(RC522) 2번 모듈 핀 설정 (배선에 맞게 변경)
NfcReader nfcReader2({
  .pin = {
    .SCK = D8,   // RC522 SCK 핀(공유)
    .MISO = D9,  // RC522 MISO 핀(공유)
    .MOSI = D10, // RC522 MOSI 핀(공유)
    .SS = D4,    // RC522 SS/SDA 핀(2번 모듈, 배선에 맞게 변경)
    .RST = D2,   // RC522 RST 핀(2번 모듈, 배선에 맞게 변경)
    .IRQ = -1,  // 사용 안 함
  },
  .task = &nfcTask2, // NFC 스캔을 돌릴 TaskRunner
  .settings = {
    .miss = 500,          // 카드 제거로 판단할 시간(ms)
    .irqMode = false,     // IRQ 모드 사용 여부
    .pollIntervalMs = 25, // 폴링 주기(ms)
    .reinitMisses = 8,    // 실패 누적 시 재초기화 횟수
  },
});

/* 
  setup()/loop()는 src/app.cpp에 정의되어 있으며
  PlatformIO/Arduino 빌드가 모든 .cpp를 함께 링크하여 자동으로 사용됩니다.
*/
