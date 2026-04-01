/*
  [파일 설명] lib/http/http.cpp
  - 이 파일은 해당 모듈의 실제 동작(초기화/루프/에러처리/리소스 정리)을 구현한다.
  - 헤더의 인터페이스 설명과 함께 읽으면 전체 흐름을 빠르게 파악할 수 있다.
*/
#include "Http.h"

Http::Http() {}

// setTimeout(): HTTP read/write timeout 설정
void Http::setTimeout(uint32_t ms) { _rwTimeoutMs = ms; }

// toMethod(): enum -> 문자열 메서드 변환
String Http::toMethod(QuestType type) {
  switch (type) {
  case Get:
    return F("GET");
  case Post:
    return F("POST");
  case Put:
    return F("PUT");
  case Delete:
    return F("DELETE");
  case Patch:
    return F("PATCH");
  }
  return F("GET");
}

/*
  ensureWorker()
  - 내부 큐와 worker 태스크를 지연 생성(lazy init)한다.
  - 첫 quest() 호출 시 초기화되므로, 불필요한 부팅 비용을 줄인다.
*/
void Http::ensureWorker() {
  if (_jobQ == nullptr) {
    // 큐 원소는 Job* 포인터다.
    _jobQ = xQueueCreate(JOB_QUEUE_LEN, sizeof(Job *));
  }
  if (_workerTask == nullptr) {
    // worker를 고정 코어에 올려 HTTP 수행 위치를 안정화한다.
    xTaskCreatePinnedToCore(&Http::workerThunk, "http_worker", 8192, this, 2,
                            &_workerTask, 1);
  }
}

void Http::workerThunk(void *pv) { static_cast<Http *>(pv)->workerLoop(); }

// URL 문자열에 scheme이 없으면 기본 http://를 붙인다.
static String ensureHttpScheme(const String &u) {
  if (u.startsWith("http://") || u.startsWith("https://")) {
    return u;
  }
  // scheme 누락 시 기본값은 http://
  return String("http://") + u;
}

// path를 "/..." 형태로 정규화한다.
static String normalizedPath(String p) {
  if (p.length() == 0)
    return "/";
  if (p[0] != '/')
    p = "/" + p;
  // 실수로 들어온 중복 슬래시를 정리한다.
  while (p.indexOf("//") != -1)
    p.replace("//", "/");
  return p;
}

/*
  workerLoop()
  - _jobQ에서 Job*을 꺼내 HTTP를 수행하고
  - 결과(Result*)를 caller 전용 replyQ로 전달한다.
*/
void Http::workerLoop() {
  for (;;) {
    Job *job = nullptr;
    if (xQueueReceive(_jobQ, &job, portMAX_DELAY) != pdTRUE)
      continue;
    if (job == nullptr)
      continue;

    Result *res = new Result();
    res->httpCode = -1;
    res->body = "";

    // 최종 URL 조합
    // 예: http://192.168.1.100:3001/hanger/connect
    const String base = ensureHttpScheme(job->req.url);
    const String path = normalizedPath(job->req.path);
    const String url = base + ":" + String(job->req.port) + path;

    Serial.println(F("[HTTP][W] job received"));
    Serial.printf("[HTTP][W] URL: %s\n", url.c_str());
    Serial.printf("[HTTP][W] Method: %s\n", toMethod(job->req.type).c_str());

    // Wi-Fi 미연결이면 바로 빈 응답으로 종료
    // 요청을 강행해도 실패하므로 빠르게 반환해 상위 로직이 계속 진행되게 한다.
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println(F("[HTTP][W] WiFi disconnected -> empty response"));
      xQueueSend(job->replyQ, &res, 0);
      delete job;
      continue;
    }

    HTTPClient http;
    http.setTimeout(_rwTimeoutMs);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    int httpCode = -1;

    switch (job->req.type) {
    case Post:
      httpCode = http.POST(job->req.body);
      break;
    case Put:
      httpCode = http.PUT(job->req.body);
      break;
    case Delete:
      if (job->req.body.length() > 0)
        httpCode = http.sendRequest("DELETE", job->req.body);
      else
        httpCode = http.sendRequest("DELETE");
      break;
    case Patch:
      httpCode = http.sendRequest("PATCH", job->req.body);
      break;
    case Get:
    default:
      httpCode = http.GET();
      break;
    }

    res->httpCode = httpCode;

    if (httpCode > 0) {
      res->body = http.getString();
      Serial.printf("[HTTP][W] response code: %d\n", httpCode);
      Serial.printf("[HTTP][W] response body: %s\n", res->body.c_str());
    } else {
      Serial.printf("[HTTP][W] request failed, code: %d\n", httpCode);
      res->body = "";
    }

    http.end();

    // caller에게 결과 전달
    // Result* 소유권은 caller(quest)로 넘어간다.
    xQueueSend(job->replyQ, &res, 0);

    // job 객체 정리
    delete job;
  }
}

/*
  quest(req)
  - 호출자는 동기 함수처럼 사용한다.
  - 내부적으로 job enqueue -> reply 대기 -> body 반환 순서로 동작한다.
*/
String Http::quest(const HttpRequest &req) {
  ensureWorker();

  // 요청별 1칸 reply 큐
  // 같은 호출의 응답만 받도록 reply 큐를 호출 단위로 분리한다.
  QueueHandle_t replyQ = xQueueCreate(1, sizeof(Result *));
  if (!replyQ)
    return "";

  Job *job = new Job();
  job->req = req;
  job->replyQ = replyQ;

  Serial.println(F("[HTTP] quest() enqueue"));
  Serial.printf("[HTTP] Method: %s\n", toMethod(req.type).c_str());

  // 큐 포화 시 드랍
  // 블로킹 대기를 하지 않는 이유: 호출부 지연 전파를 막기 위함
  if (xQueueSend(_jobQ, &job, 0) != pdTRUE) {
    Serial.println(F("[HTTP] jobQ full -> drop"));
    delete job;
    vQueueDelete(replyQ);
    return "";
  }

  // reply 대기 (요청 타임아웃 + 여유 1초)
  // worker가 timeout 직전에 반환해도 수신할 수 있도록 여유를 둔다.
  Result *res = nullptr;
  TickType_t waitTicks = pdMS_TO_TICKS(_rwTimeoutMs + 1000);
  if (xQueueReceive(replyQ, &res, waitTicks) != pdTRUE || res == nullptr) {
    Serial.println(F("[HTTP] response timeout"));
    vQueueDelete(replyQ);
    return "";
  }

  String body = res->body;

  delete res;
  vQueueDelete(replyQ);

  Serial.println(F("[HTTP] quest() done"));
  return body;
}

