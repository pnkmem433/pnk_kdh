#ifndef Http_H
#define Http_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

/*
  ==================================================================================
  Http 모듈 상세 설명
  ==================================================================================
  [겉보기 API]
  quest(req) -> String(body)
  호출부에서는 동기 함수처럼 사용한다.

  [실제 내부 동작]
  1) caller가 Job을 생성해 _jobQ에 넣는다.
  2) workerTask가 Job을 꺼내 HTTP 요청 수행
  3) 결과(Result*)를 caller의 replyQ로 전달
  4) caller는 timeout까지 기다렸다가 body를 받는다.

  [이 구조의 장점]
  - 네트워크 I/O를 한 태스크에서 직렬 처리해 디버깅이 쉽다.
  - 호출부가 HTTPClient 생명주기/예외 처리를 몰라도 된다.
  - 타임아웃/큐포화 정책을 모듈 내부에서 통일할 수 있다.
  ==================================================================================
*/

enum QuestType { Get, Post, Put, Delete, Patch };

struct HttpRequest {
  QuestType type;
  // 호스트/IP. "http://"가 없으면 내부에서 자동 보정한다.
  String url;
  int port;
  // "/api/..." 형식 권장. 슬래시 누락 시 자동 보정한다.
  String path;
  // POST/PUT/PATCH/DELETE body
  String body;
};

class Http {
public:
  Http();

  // --------------------------------------------------------------------------
  // setTimeout(ms)
  // - HTTPClient timeout과 caller 대기 시간 계산에 사용된다.
  // --------------------------------------------------------------------------
  void setTimeout(uint32_t ms);

  // --------------------------------------------------------------------------
  // quest(httpQuest)
  // [입력]
  // - method/url/port/path/body
  // [출력]
  // - 성공: 응답 body 문자열
  // - 실패: 빈 문자열
  // [실패 케이스 예시]
  // - Wi-Fi 미연결
  // - 큐 포화
  // - 응답 대기 timeout
  // --------------------------------------------------------------------------
  String quest(const HttpRequest &httpQuest);

private:
  // worker 큐에 넣는 작업 단위
  struct Job {
    HttpRequest req;
    // caller마다 1칸짜리 reply 큐를 만들어 결과를 돌려준다.
    QueueHandle_t replyQ;
  };

  struct Result {
    int httpCode;
    String body;
  };

  static void workerThunk(void *pv);
  void workerLoop();

  void ensureWorker();
  String toMethod(QuestType type);

private:
  uint32_t _rwTimeoutMs = 5000;

  TaskHandle_t _workerTask = nullptr;
  QueueHandle_t _jobQ = nullptr;

  // 한 번에 대기 가능한 HTTP 작업 수
  static constexpr uint16_t JOB_QUEUE_LEN = 8;
};

#endif
