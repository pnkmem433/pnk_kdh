#ifndef UUID_H
#define UUID_H

#include <Arduino.h>
#include <EEPROM.h>

// UUID(36문자) 저장을 위해 EEPROM 64바이트 사용
#define EEPROM_SIZE 64
#define UUID_LENGTH 36
#define UUID_ADDR 0

/*
  ==================================================================================
  Uuid 모듈 상세 설명
  ==================================================================================
  [문제]
  장치가 재부팅될 때마다 ID가 바뀌면 백엔드에서 동일 장치 추적이 불가능해진다.

  [해결]
  - UUID를 EEPROM에 저장해 영구 유지
  - 부팅 시 EEPROM 값을 읽고 형식을 검사
  - 유효하지 않으면 새 UUID(v4) 생성 후 저장

  [UUID를 반드시 쓰는 이유]
  - 백엔드에서 장치별 이벤트를 정확히 분리하기 위해
  - MQTT 토픽/콜백에서 장치 주체를 식별하기 위해
  - 전원 재시작 이후에도 동일 장치로 관리되기 위해
  - OTA 운영 시 장치 번호를 수동 관리하지 않고,
    공통 펌웨어 배포 + 장치별 자동 식별/추적을 가능하게 하기 위해

  [결과]
  장치 교체 전까지 동일 UUID를 지속적으로 사용한다.
  ==================================================================================
*/
class Uuid {
public:
  Uuid();

  // EEPROM에서 UUID를 로드하고 필요 시 생성/저장
  void begin();

  // 현재 UUID 반환
  String load();

private:
  // UUID v4 문자열 생성
  String generator();

  // 현재 uuid 문자열 유효성 검사
  bool isUuid();

  // EEPROM에 UUID 저장 + commit
  void saveUUID();

  String uuid;
};

#endif
