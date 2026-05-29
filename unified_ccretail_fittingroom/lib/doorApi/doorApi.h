/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - Door REST API 호출 계약 정의.
*/
#ifndef DOOR_QUEST_H
#define DOOR_QUEST_H

#include "http.h"
#include <vector>

struct DoorApiResult {
  int statusCode;
  String body;
  bool alreadyMatched;
};

/*
  DoorApi
  - 목적: CC-Retail_mvp0.0 피팅룸 문 로그 / 세션 아이템 REST API 래퍼.
  - 엔드포인트:
    - PATCH /api/fitting-rooms/{id}/door-status {"isDoorClosed": bool}
    - POST  /api/fitting-rooms/{id}/session-items {"productVariantSkus": [...]}
  - 실패 시 동작:
    - 내부 Http::quest() 실패 시 빈 응답을 받을 수 있으며, 현재 구현은 예외 없이 다음 흐름을 계속 진행한다.
*/
class DoorApi {
public:
  /*
    DoorApi(http, url, port)
    - 입력: Http 전송기 포인터, 서버 호스트, 포트
    - 출력: 객체 생성
    - 부작용: 내부 전송 대상 정보 저장
    - 주의: http 포인터가 유효해야 하며, 수명은 DoorApi보다 길어야 한다.
  */
  DoorApi(Http *http, String url, int port);

  /*
    begin()
    - 입력/출력: 없음
    - 부작용: 내부 Http begin 호출
    - 주의: 네트워크 링크(WiFi/Ethernet)는 호출 전에 상위에서 먼저 준비되어야 한다.
  */
  void begin();

  /*
    setFittingRoomId(fittingRoomId)
    - 입력: 피팅룸 식별 ID
    - 출력: 없음
    - 부작용: 이후 open/close/sendTags의 URL 경로가 변경된다.
    - 주의: 미설정(0) 상태로 요청하면 잘못된 API 경로를 호출할 수 있다.
  */
  void setFittingRoomId(int fittingRoomId);

  /*
    openDoor()
    - 입력/출력: 없음
    - 부작용: PATCH /api/fitting-rooms/{id}/door-status {"isDoorClosed": false} 전송
    - 주의: 전송 실패 시 재시도 로직은 상위 계층에서 별도로 설계해야 한다.
  */
  DoorApiResult openDoor();

  /*
    closeDoor()
    - 입력/출력: 없음
    - 부작용: PATCH /api/fitting-rooms/{id}/door-status {"isDoorClosed": true} 전송
    - 주의: 문 상태 동기화는 네트워크 품질에 따라 지연될 수 있다.
  */
  DoorApiResult closeDoor();

  /*
    sendTags(tags)
    - 입력: SKU 문자열 목록(product_variant.sku)
    - 출력: 없음
    - 부작용: POST /api/fitting-rooms/{id}/session-items {"productVariantSkus":[...]} 전송
    - 주의: 빈 목록 전송 정책은 호출부에서 결정(현재 main은 빈 목록이면 호출하지 않음)
  */
  DoorApiResult sendTags(std::vector<String> tags);

private:
  Http *http;

  String url;
  int port;

  int fittingRoomId;

  DoorApiResult patchDoorStatus(bool isClosed);
  String productVariantSkusJson(std::vector<String> tags);
};

#endif

