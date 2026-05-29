/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - HTTP 요청 생성/타임아웃/응답 수집 구현.
*/
#include "http.h"

#include <UIPEthernet.h>

#ifndef HTTP_VERBOSE_HEADERS
#define HTTP_VERBOSE_HEADERS 0
#endif

#ifndef HTTP_VERBOSE_CONNECTION
#define HTTP_VERBOSE_CONNECTION 0
#endif

namespace {
const char *linkStatusToString(EthernetLinkStatus status) {
  switch (status) {
  case LinkON:
    return "LinkON";
  case LinkOFF:
    return "LinkOFF";
  default:
    return "Unknown";
  }
}

const char *hardwareStatusToString(EthernetHardwareStatus status) {
  switch (status) {
  case EthernetNoHardware:
    return "EthernetNoHardware";
  case EthernetW5100:
    return "EthernetW5100-compatible";
  default:
    return "EthernetHardwareDetected";
  }
}

String firstHttpLine(const String &response) {
  int end = response.indexOf("\r\n");
  if (end < 0) {
    return response;
  }
  return response.substring(0, end);
}

void printEthernetSnapshot(const char *stage, Client *client) {
  Serial.printf(
      "[HTTP][diag] %s | link=%s | hw=%s | ip=%s | gw=%s | dns=%s | connected=%d | available=%d\n",
      stage, linkStatusToString(UIPEthernet.linkStatus()),
      hardwareStatusToString(UIPEthernet.hardwareStatus()),
      UIPEthernet.localIP().toString().c_str(),
      UIPEthernet.gatewayIP().toString().c_str(),
      UIPEthernet.dnsServerIP().toString().c_str(), client->connected(),
      client->available());
}
} // namespace

/*
  Http(client)
  - 입력: 통신에 사용할 Client 구현체 포인터
  - 부작용: 내부 포인터 저장
*/
Http::Http(Client *client) : client(client), lastStatusCode(-1) {}

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
  lastStatusCode = -1;
  client->setTimeout(1000);
  unsigned long t_start = millis();

  String path = req.path;
  if (path.length() == 0 || path[0] != '/')
    path = "/" + path;

  Serial.printf("[HTTP][diag] request method=%s host=%s port=%d path=%s body=%u\n",
                toMethod(req.type).c_str(), req.url.c_str(), req.port,
                path.c_str(), req.body.length());
  printEthernetSnapshot("before-connect", client);

  if (HTTP_VERBOSE_CONNECTION) {
    Serial.printf("[HTTP] Connecting to %s:%d ...\n",
                  req.url.c_str(), req.port);
  }

  if (!client->connect(req.url.c_str(), req.port)) {
    Serial.println(F("[HTTP] Connection failed"));
    Serial.printf("[HTTP][diag] connect-failed host=%s port=%d\n",
                  req.url.c_str(), req.port);
    printEthernetSnapshot("after-connect-fail", client);
    return "";
  }

  Serial.printf("[HTTP] Connected: %s:%d, ready to send request.\n",
                req.url.c_str(), req.port);
  printEthernetSnapshot("after-connect", client);

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

  const unsigned long REQ_TIMEOUT_MS = 8000;
  unsigned long t0 = millis();

  while (!client->available()) {
    if (!client->connected()) break;

    if (millis() - t0 > REQ_TIMEOUT_MS) {
      Serial.println(F("[HTTP] Timeout waiting for first response byte"));
      Serial.printf("[HTTP][diag] timeout-before-status elapsed=%lu\n",
                    millis() - t_start);
      printEthernetSnapshot("timeout-before-status", client);
      client->stop();
      return "";
    }
    delay(5);
  }

  if (!client->available() && !client->connected()) {
    Serial.println(F("[HTTP][diag] socket-closed-before-status"));
    printEthernetSnapshot("closed-before-status", client);
  }

  String statusLine = client->readStringUntil('\n');
  String rawStatusLine = statusLine;
  statusLine.trim();
  Serial.printf("[HTTP][diag] raw-status-line=%s\n", rawStatusLine.c_str());
  if (HTTP_VERBOSE_HEADERS) {
    Serial.printf("[HTTP] status line: %s\n", statusLine.c_str());
  }
  int sp1 = statusLine.indexOf(' ');
  if (sp1 > 0) {
    int sp2 = statusLine.indexOf(' ', sp1 + 1);
    if (sp2 < 0) sp2 = statusLine.length();
    resultStatus =
        statusLine.substring(sp1 + 1, sp2).toInt();
  } else {
    String rawBytes = "";
    for (size_t i = 0; i < statusLine.length(); ++i) {
      if (i > 0) {
        rawBytes += " ";
      }
      char buf[4];
      snprintf(buf, sizeof(buf), "%02X",
               static_cast<unsigned char>(statusLine[i]));
      rawBytes += buf;
    }
    Serial.printf("[HTTP] Raw status line text: '%s'\n", statusLine.c_str());
    Serial.printf("[HTTP] Raw status line hex : %s\n", rawBytes.c_str());
    Serial.println(F("[HTTP] Failed to parse HTTP status line"));
    printEthernetSnapshot("status-parse-failed", client);
  }

  unsigned long headerStarted = millis();
  while (client->connected()) {
    String h = client->readStringUntil('\n');
    h.trim();
    if (HTTP_VERBOSE_HEADERS && h.length() > 0) {
      Serial.printf("[HTTP] header: %s\n", h.c_str());
    }
    if (h == "\r" || h.length() <= 1) break;
    if (millis() - headerStarted > REQ_TIMEOUT_MS) {
      Serial.println(F("[HTTP] Timeout while reading headers"));
      Serial.println(F("[HTTP][diag] header-read-timeout"));
      break;
    }
  }

  while (client->available()) {
    response += (char)client->read();
  }

  unsigned long t1 = millis();
  while (client->connected()) {
    while (client->available())
      response += (char)client->read();

    if (millis() - t1 > 1000)
      break;

    delay(2);
  }

  client->stop();

  unsigned long t_end = millis();
  Serial.printf("[HTTP] Done status=%d elapsed=%lu ms\n",
                resultStatus, (t_end - t_start));
  Serial.printf("[HTTP][diag] response-bytes=%u first-line=%s\n", response.length(),
                firstHttpLine(response).c_str());
  lastStatusCode = resultStatus;

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

int Http::getLastStatusCode() const { return lastStatusCode; }
