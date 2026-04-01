#include "HttpManageX.h"

HttpManageX::HttpManageX(String url, String url2) : _url(url), _url2(url2) {}

void HttpManageX::setUuid(String uuid) { _uuid = uuid; }

bool HttpManageX::saveDataArray(std::vector<ThermohygrometerData> &dataVector) {
  // HTTPClient 객체 생성
  HTTPClient http;

  http.setTimeout(3000); // 3초 타임아웃 설정

  // JSON 형식으로 데이터 배열 만들기
  String jsonData = "{\"datas\":[";

  // 벡터의 각 데이터를 JSON 형식으로 변환하여 추가
  for (size_t i = 0; i < dataVector.size(); ++i) {
    const ThermohygrometerData &data = dataVector[i];
    DateTime time = data.dateTime;
    String dataItem = "{\"deviceUUID\":\"" + _uuid + "\"," +
                      "\"value\":{\"humi\":" + String(data.humi) +
                      ",\"temp\":" + String(data.temp) + "}," +
                      "\"createAt\":\"" + time.toString() + "\"}";

    // 마지막 데이터는 쉼표를 추가하지 않음
    if (i != dataVector.size() - 1) {
      jsonData += dataItem + ",";
    } else {
      jsonData += dataItem;
    }
  }

  jsonData += "]}"; // JSON 배열 닫기

  // URL 설정
  http.begin(_url);

  // POST 요청 헤더 설정 (JSON 형식)
  http.addHeader("Content-Type", "application/json");

  // POST 요청 보내기
  int httpCode = http.POST(jsonData);

  // 응답 처리
  bool result = false;
  if (httpCode == HTTP_CODE_OK ||
      httpCode == HTTP_CODE_CREATED) { // 응답 코드가 200이면 성공
    result = true;
  }

  // HTTP 연결 종료
  http.end();

  // 성공 여부 반환
  return result;
}

bool HttpManageX::saveData(ThermohygrometerData data) {
  std::vector<ThermohygrometerData> dataVector = {data};
  return saveDataArray(dataVector);
}

bool HttpManageX::sendControlLog(int controlType) {
  HTTPClient http;

  http.setTimeout(3000);

  http.begin(_url2);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST("{\"uuid\": \"" + _uuid +
                           "\", \"err_type\": " + String(controlType) + "}");

  bool result = false;
  if (httpCode == HTTP_CODE_OK ||
      httpCode == HTTP_CODE_CREATED) { // 응답 코드가 200이면 성공
    result = true;
  }

  http.end();

  return result;
}