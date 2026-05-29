/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - HTTP 요청 타입과 전송 인터페이스 정의.
*/
#ifndef Http_H
#define Http_H

#include <Arduino.h>
#include <Client.h>

enum QuestType { Get, Post, Put, Delete, Patch };

/*
  HttpRequest
  - url/port/path/body로 간단한 HTTP 요청 구성.
*/
struct HttpRequest {
  QuestType type;
  String url;
  int port;
  String path;
  String body;
};

/*
  Http
  - 목적: 주입받은 Client(WiFiClient/EthernetClient)로 경량 HTTP 요청 수행.
  - 설계 이유: 네트워크 계층을 추상화해 상위 DoorApi가 전송 매체 의존도를 줄이도록 함.
*/
class Http {
private:
  Client *client;
  String toMethod(QuestType type);

public:
  /*
    Http(client)
    - 입력: 네트워크 Client 구현체 포인터(WiFiClient/EthernetClient)
    - 출력: 객체 생성
    - 부작용: 포인터 저장
    - 주의: client 수명은 Http 객체보다 길어야 한다.
  */
  Http(Client *client);

  /*
    begin()
    - 입력/출력: 없음
    - 부작용: 현재 구현은 없음(확장 포인트)
    - 주의: 네트워크 링크 준비는 상위에서 수행해야 한다.
  */
  void begin();

  /*
    quest(httpQuest)
    - 입력: 요청 메서드/호스트/포트/경로/바디
    - 출력: 응답 body 문자열(실패 시 빈 문자열)
    - 부작용: 소켓 connect/write/read/stop 수행
    - 실패/주의:
      - timeout 또는 connect 실패 시 빈 문자열 반환
      - 호출자는 상태코드가 아닌 body 기준 처리를 하고 있으므로 운영 설계상 유의 필요
  */
  String quest(HttpRequest httpQuest);
};

#endif


