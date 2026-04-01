/*
  [파일 설명] lib/uuid/uuid.cpp
  - 이 파일은 해당 모듈의 실제 동작(초기화/루프/에러처리/리소스 정리)을 구현한다.
  - 헤더의 인터페이스 설명과 함께 읽으면 전체 흐름을 빠르게 파악할 수 있다.
*/
#include "Uuid.h"

Uuid::Uuid() {}

// begin(): EEPROM에서 UUID를 읽고 유효성 검사 후 필요 시 새로 생성한다.
void Uuid::begin() {
  EEPROM.begin(EEPROM_SIZE);
  delay(100);

  // EEPROM에서 36바이트를 읽어 UUID 문자열로 복원
  // 문자열 끝에 '\0'을 직접 넣어 안전하게 String 변환한다.
  char uuidc[UUID_LENGTH + 1];
  for (int i = 0; i < UUID_LENGTH; i++) {
    uuidc[i] = EEPROM.read(UUID_ADDR + i);
  }
  uuidc[UUID_LENGTH] = '\0';

  uuid = String(uuidc);

  // 유효한 UUID가 아니면 새로 생성 후 저장
  // (초기 출하 상태, EEPROM 깨짐, 포맷 불일치 등)
  if (!isUuid()) {
    uuid = generator();
    saveUUID();
  }
}

String Uuid::load() { return uuid; }

// generator(): RFC4122 v4 형식 UUID 문자열 생성
String Uuid::generator() {
  uint8_t b[16];
  esp_fill_random(b, sizeof(b)); // 하드웨어 RNG 사용

  // RFC 4122 v4 세팅
  b[6] = (b[6] & 0x0F) | 0x40; // version 4
  b[8] = (b[8] & 0x3F) | 0x80; // variant 10xx

  char s[37];
  snprintf(
      s, sizeof(s),
      "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
      b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11],
      b[12], b[13], b[14], b[15]);
  return String(s);
}

// isUuid(): 문자열이 UUID 포맷인지 검사
bool Uuid::isUuid() {
  if (uuid.length() != UUID_LENGTH)
    return false;

  // xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx 형식 검사
  // 하이픈 위치와 hex 문자 여부를 모두 확인한다.
  for (int i = 0; i < uuid.length(); i++) {
    char c = uuid.charAt(i);
    if (i == 8 || i == 13 || i == 18 || i == 23) {
      if (c != '-')
        return false;
    } else {
      if (!isHexadecimalDigit(c))
        return false;
    }
  }
  return true;
}

// saveUUID(): 현재 UUID를 EEPROM에 영구 저장
void Uuid::saveUUID() {
  for (int i = 0; i < UUID_LENGTH; i++) {
    EEPROM.write(UUID_ADDR + i, uuid.charAt(i));
  }
  EEPROM.commit();
}

