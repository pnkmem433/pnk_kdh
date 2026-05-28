/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - fittingRoom door/tags API 호출 로직 구현.
*/
/*
  모듈 목적:
  - Door 관련 백엔드 REST API 호출을 캡슐화한다.

  입력:
  - fittingRoomId, tags 목록

  출력:
  - HTTP PATCH/POST 요청 전송

  다른 모듈과의 관계:
  - Http 모듈을 통해 실제 전송을 수행하며, URL/포트/경로 구성만 담당한다.

  실패 시 동작:
  - Http 전송 실패 시 현재 구현은 예외를 던지지 않고 상위 흐름을 계속 진행한다.
*/
#include "doorApi.h"

/*
  DoorApi(http, url, port)
  - 입력: HTTP 전송기, 서버 호스트, 포트
  - 부작용: 전송 대상 기본 정보 저장
*/
DoorApi::DoorApi(Http *http, String url, int port)
    : http(http), url(url), port(port), fittingRoomId(0) {}

/*
  begin()
  - 내부 http begin을 위임 호출.
*/
void DoorApi::begin() { http->begin(); }

/*
  setFittingRoomId(id)
  - 향후 모든 API 경로의 room id를 설정한다.
*/
void DoorApi::setFittingRoomId(int fittingRoomId) {
  this->fittingRoomId = fittingRoomId;
}

/*
  openDoor()
  - PATCH /fittingrooms/{id}/door {"isClosed": false} 전송.
*/
void DoorApi::openDoor() {
  // payload 필드 의미:
  // - isClosed=false: 서버에 "문 열림 상태"를 반영한다.
  Serial.println("DoorApi: Sending open door request.");
  http->quest({
      .type = QuestType::Patch,
      .url = url,
      .port = port,
      .path = "/fittingrooms/" + String(fittingRoomId) + "/door",
      .body = "{\"isClosed\": false}",
  });
}

/*
  closeDoor()
  - PATCH /fittingrooms/{id}/door {"isClosed": true} 전송.
*/
void DoorApi::closeDoor() {
  // payload 필드 의미:
  // - isClosed=true: 서버에 "문 닫힘 상태"를 반영한다.
  Serial.println("DoorApi: Sending close door request.");
  http->quest({
      .type = QuestType::Patch,
      .url = url,
      .port = port,
      .path = "/fittingrooms/" + String(fittingRoomId) + "/door",
      .body = "{\"isClosed\": true}",
  });
}

/*
  sendTags(tags)
  - POST /fittingrooms/{id}/tags {"tags":[...]} 전송.
*/
void DoorApi::sendTags(std::vector<String> tags) {
  // tags payload 의미:
  // - 스캔 윈도우에서 최종 확정된 태그 코드 목록
  // - 중복/노이즈 정리는 호출부(main)의 필터 정책을 따른다.
  Serial.println("DoorApi: Sending tags.");
  http->quest({
      .type = QuestType::Post,
      .url = url,
      .port = port,
      .path = "/fittingrooms/" + String(fittingRoomId) + "/tags",
      .body = tagsToJson(tags),
  });
}

/*
  tagsToJson(tags)
  - 태그 문자열 목록을 API 규약 JSON 문자열로 직렬화한다.
*/
String DoorApi::tagsToJson(std::vector<String> tags) {
  String json = "{\"tags\": [";
  for (size_t i = 0; i < tags.size(); ++i) {
    json += "\"" + tags[i] + "\"";
    if (i < tags.size() - 1) {
      json += ", ";
    }
  }
  json += "]}";
  return json;
}


