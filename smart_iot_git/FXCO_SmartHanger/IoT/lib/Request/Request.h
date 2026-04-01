#ifndef SMART_HANGER_REQUEST_H
#define SMART_HANGER_REQUEST_H

#include "http.h"
#include <ArduinoJson.h>

/*
  ==================================================================================
  Request 모듈 상세 설명
  ==================================================================================
  [역할]
  Smart Hanger 백엔드 REST API를 "도메인 메서드"로 캡슐화한다.
  - begin(uuid)
  - pickup()
  - pickdown(locationUuid)
  - timeCheckStart()/timeCheckEnd()

  [왜 필요한가]
  main.cpp에서 URL/path/body를 매번 조립하면
  - 코드 중복 증가
  - 실수 가능성 증가
  - 엔드포인트 변경 시 수정 범위 증가
  Request가 이를 한 곳에서 관리한다.
  ==================================================================================
*/

struct RequestInfo {
  Http *http;
  String url;
  int port;
};

class Request {
public:
  explicit Request(RequestInfo info);

  // 장치 연결(등록) API 호출
  void begin(String uuid);

  // connect 응답(JSON) 존재 여부
  bool canLoadClothData();

  // connect 응답 원문(JSON)
  String loadClothData();

  // 행거 픽업/픽다운 이벤트 전송
  void pickup();
  void pickdown(String locationUuid);

  // 이벤트 처리 시간 측정용 시작/종료 시각 기록
  void timeCheckStart(uint64_t startTime);
  void timeCheckEnd(uint64_t endTime);

  // 현재 rack(leg) UUID 저장
  void setRackUuid(String rackUuid);

private:
  Http *http;
  String uuid;
  int port;
  String url;
  uint64_t startTime;

  // /hanger/connect 응답 캐시
  String lastConnectResponse;

  // 현재 감지된 rack UUID
  String rackUuid;
};

#endif
