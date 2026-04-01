/*
  [파일 설명] lib/Request/Request.cpp
  - 이 파일은 해당 모듈의 실제 동작(초기화/루프/에러처리/리소스 정리)을 구현한다.
  - 헤더의 인터페이스 설명과 함께 읽으면 전체 흐름을 빠르게 파악할 수 있다.
*/
#include "Request.h"
#include <ArduinoJson.h>

Request::Request(RequestInfo info)
    : http(info.http), url(info.url), port(info.port), startTime(0) {}

// begin(): /hanger/connect 호출 후 응답(JSON)을 캐시한다.
void Request::begin(String uuid) {
  this->uuid = uuid;

  // 장치 등록 + 초기 의류 데이터 요청
  // connect 응답은 이후 UI 초기 모델(가격/색상/URL)로 사용된다.
  String res = http->quest({
      .type = QuestType::Post,
      .url = url,
      .port = port,
      .path = "/hanger/connect",
      .body = "{\"uuid\":\"" + uuid + "\"}",
  });

  // 비어있지 않은 응답만 캐시한다.
  // "{}" 같은 빈 객체만 받은 경우는 데이터 없음으로 본다.
  if (res.length() > 2) {
    lastConnectResponse = res;
  }
}

bool Request::canLoadClothData() {
  // 디버깅 편의를 위해 원문을 출력한다.
  Serial.println(lastConnectResponse);
  return lastConnectResponse.length();
}

String Request::loadClothData() { return lastConnectResponse; }

// pickup(): 카드 제거 이벤트를 서버에 전송
void Request::pickup() {
  http->quest({
      .type = QuestType::Post,
      .url = url,
      .port = port,
      .path = "/hanger/pickup",
      .body = "{\"uuid\":\"" + uuid + "\"}",
  });
}

// pickdown(): 카드 감지 이벤트(랙 UUID 포함)를 서버에 전송
void Request::pickdown(String locationUuid) {
  http->quest({
      .type = QuestType::Post,
      .url = url,
      .port = port,
      .path = "/hanger/pickdown",
      .body = "{\"uuid\":\"" + uuid + "\",\"pickdownUuid\":\"" +
              locationUuid + "\"}",
  });
}

// timeCheckStart(): 이벤트 처리 지연 측정 시작 시각 저장
void Request::timeCheckStart(uint64_t startTime) { this->startTime = startTime; }

// timeCheckEnd(): 측정 종료 시각과 함께 /time-tracking 전송
void Request::timeCheckEnd(uint64_t endTime) {
  // 시작/종료 시각이 모두 유효할 때만 전송
  // 이벤트 처리 왕복 시간 추적용 endpoint
  if (startTime == 0 || endTime == 0) {
    return;
  }

  http->quest({.type = QuestType::Post,
               .url = url,
               .port = port,
               .path = "/time-tracking",
               .body = "{\"uuid\":\"" + uuid + "\",\"startTime\":" +
                       String(startTime) + ",\"endTime\":" + String(endTime) +
                       ",\"legUuid\":\"" + rackUuid + "\"}"});
}

// setRackUuid(): 현재 감지 rack UUID를 저장
void Request::setRackUuid(String rackUuid) { this->rackUuid = rackUuid; }

