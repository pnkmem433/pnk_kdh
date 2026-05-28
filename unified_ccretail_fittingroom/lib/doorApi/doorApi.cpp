/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - fittingRoom door-status / session-items API 호출 로직 구현.
*/
/*
  모듈 목적:
  - CC-Retail_mvp0.0 Nest 백엔드의 피팅룸 Door / session-items API 호출을 캡슐화한다.

  입력:
  - fittingRoomId, RFID에서 확정된 SKU 문자열 목록

  출력:
  - HTTP PATCH/POST 요청 전송

  다른 모듈과의 관계:
  - Http 모듈을 통해 실제 전송을 수행하며, URL/포트/경로 구성만 담당한다.

  실패 시 동작:
  - Http 전송 실패 시 현재 구현은 예외를 던지지 않고 상위 흐름을 계속 진행한다.
*/
#include "doorApi.h"

namespace {
String previewBody(const String &body) {
  const size_t maxLen = 120;
  if (body.length() <= maxLen) {
    return body;
  }
  return body.substring(0, maxLen) + "...";
}

bool bodyContains(const String &body, const char *needle) {
  return body.indexOf(needle) >= 0;
}
} // namespace

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
  - PATCH /api/fitting-rooms/{id}/door-status {"isDoorClosed": false} 전송.
  - 백엔드는 fitting_room_door_log 에 openedAt 레코드를 생성한다.
*/
DoorApiResult DoorApi::openDoor() { return patchDoorStatus(false); }

/*
  closeDoor()
  - PATCH /api/fitting-rooms/{id}/door-status {"isDoorClosed": true} 전송.
  - 백엔드는 미종료 열림 로그에 closedAt 을 기록한다.
*/
DoorApiResult DoorApi::closeDoor() { return patchDoorStatus(true); }

DoorApiResult DoorApi::patchDoorStatus(bool isClosed) {
  const String path =
      "/api/fitting-rooms/" + String(fittingRoomId) + "/door-status";
  const String body =
      isClosed ? "{\"isDoorClosed\":true}" : "{\"isDoorClosed\":false}";
  Serial.printf("DoorApi: PATCH door-status %s %s\n", path.c_str(),
                body.c_str());
  String response = http->quest({
      .type = QuestType::Patch,
      .url = url,
      .port = port,
      .path = path,
      .body = body,
  });
  DoorApiResult result = {
      .statusCode = http->getLastStatusCode(),
      .body = response,
      .alreadyMatched = false,
  };

  if (result.statusCode == 400) {
    if (!isClosed && bodyContains(response, "Door is already open")) {
      result.alreadyMatched = true;
    }
    if (isClosed && bodyContains(response, "already closed")) {
      result.alreadyMatched = true;
    }
  }

  Serial.printf("DoorApi: PATCH result status=%d body=%s\n", result.statusCode,
                previewBody(response).c_str());
  return result;
}

/*
  sendTags(tags)
  - POST /api/fitting-rooms/{id}/session-items {"productVariantSkus":[...]} 전송.
  - tags 벡터에는 백엔드 product_variant.sku 와 일치하는 문자열을 넣는다.
*/
DoorApiResult DoorApi::sendTags(std::vector<String> tags) {
  const String path =
      "/api/fitting-rooms/" + String(fittingRoomId) + "/session-items";
  const String body = productVariantSkusJson(tags);
  Serial.printf("DoorApi: POST session-items %s %s\n", path.c_str(),
                body.c_str());
  String response = http->quest({
      .type = QuestType::Post,
      .url = url,
      .port = port,
      .path = path,
      .body = body,
  });
  DoorApiResult result = {
      .statusCode = http->getLastStatusCode(),
      .body = response,
      .alreadyMatched = false,
  };
  Serial.printf("DoorApi: POST result status=%d body=%s\n", result.statusCode,
                previewBody(response).c_str());
  return result;
}

/*
  productVariantSkusJson(tags)
  - SKU 문자열 목록을 CreateFittingRoomItemsBySkuDto JSON 으로 직렬화한다.
*/
String DoorApi::productVariantSkusJson(std::vector<String> tags) {
  String json = "{\"productVariantSkus\": [";
  for (size_t i = 0; i < tags.size(); ++i) {
    json += "\"" + tags[i] + "\"";
    if (i < tags.size() - 1) {
      json += ", ";
    }
  }
  json += "]}";
  return json;
}
