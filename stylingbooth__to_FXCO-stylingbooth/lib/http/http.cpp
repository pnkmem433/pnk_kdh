/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - HTTP 요청 생성/타임아웃/응답 수집 구현.
*/
#include "http.h"

/*
  Http(client)
  - 입력: 통신에 사용할 Client 구현체 포인터
  - 부작용: 내부 포인터 저장
*/
Http::Http(Client *client) : client(client) {}

/*
  begin()
  - 현재 구현은 no-op.
  - 의미: 상위 계층에서 호출 순서를 통일하기 위한 인터페이스 유지 지점.
*/
void Http::begin() {
  // 별도 초기화 없음(네트워크 세션은 상위에서 준비)
}

/*
  quest()
  - 실패 처리 정책:
    - connect 실패/timeout 시 빈 문자열 반환
    - 상태코드는 로그로만 남기고 호출자는 body 기준으로 처리
  - 타임아웃:
    - 8000ms: 서버 응답 대기 상한
*/
String Http::quest(const HttpRequest req) {
  int resultStatus = -1;
  String response = "";

  Serial.println(F("[HTTP] quest() start"));
  unsigned long t_start = millis();

  String path = req.path;
  if (path.length() == 0 || path[0] != '/')
    path = "/" + path;

  Serial.printf("[HTTP] Connecting to %s:%d ...\n",
                req.url.c_str(), req.port);

  if (!client->connect(req.url.c_str(), req.port)) {
    Serial.println(F("[HTTP] Connection failed"));
    return "";
  }

  Serial.println(F("[HTTP] Connected"));

  String method = toMethod(req.type);
  const bool hasBody =
      (req.type == Post || req.type == Put || req.type == Patch);
  const int contentLength = hasBody ? req.body.length() : 0;

  client->print(method);
  client->print(" ");
  client->print(path);
  client->println(F(" HTTP/1.1"));

  client->print(F("Host: "));
  client->print(req.url);
  client->print(F(":"));
  client->println(req.port);

  client->println(F("Connection: close"));
  client->println(F("Accept: application/json"));

  if (hasBody && contentLength > 0) {
    client->println(F("Content-Type: application/json"));
    client->print(F("Content-Length: "));
    client->println(contentLength);
    client->println();
    client->print(req.body);
  } else {
    client->println();
  }

  const unsigned long REQ_TIMEOUT_MS = 3000;
  unsigned long t0 = millis();

  while (!client->available()) {
    if (!client->connected()) break;

    if (millis() - t0 > REQ_TIMEOUT_MS) {
      Serial.println(F("[HTTP] Timeout"));
      client->stop();
      return "";
    }
    delay(5);
  }

  String statusLine = client->readStringUntil('\n');
  int sp1 = statusLine.indexOf(' ');
  if (sp1 > 0) {
    int sp2 = statusLine.indexOf(' ', sp1 + 1);
    if (sp2 < 0) sp2 = statusLine.length();
    resultStatus =
        statusLine.substring(sp1 + 1, sp2).toInt();
  }

  while (client->connected()) {
    String h = client->readStringUntil('\n');
    if (h == "\r" || h.length() <= 1) break;
  }

  while (client->available()) {
    response += (char)client->read();
  }

  unsigned long t1 = millis();
  while (client->connected()) {
    while (client->available())
      response += (char)client->read();

    if (millis() - t1 > 200)
      break;

    delay(2);
  }

  client->stop();

  unsigned long t_end = millis();
  Serial.printf("[HTTP] Done status=%d elapsed=%lu ms\n",
                resultStatus, (t_end - t_start));

  return response;
}

/*
  toMethod(type)
  - QuestType enum을 HTTP 메서드 문자열로 변환한다.
*/
String Http::toMethod(QuestType type) {
  switch (type) {
  case Patch:  return "PATCH";
  case Get:    return "GET";
  case Post:   return "POST";
  case Put:    return "PUT";
  case Delete: return "DELETE";
  default:     return "GET";
  }
}
