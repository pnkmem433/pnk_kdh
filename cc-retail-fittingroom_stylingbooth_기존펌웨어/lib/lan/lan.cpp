/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - Ethernet mutex 보호 구간과 DNS/링크 처리 구현.
*/
/*
  모듈 목적:
  - UIPEthernet 기반 유선 네트워크 유틸리티를 제공한다.

  입력:
  - 도메인 문자열, Ethernet callback

  출력:
  - DNS 결과 IP, 링크 상태, callback 실행 결과

  동시성/보호 전략:
  - _ethernetMutex로 DNS/링크점검/클라이언트 사용 구간을 직렬화해 레이스를 방지한다.

  실패 시 동작:
  - mutex 획득 실패 시 false/0.0.0.0 반환
  - DNS 실패 시 0.0.0.0 반환
*/
#include "lan.h"
#include <ESP.h>

/*
  LAN(cs_pin)
  - 입력: ENC28J60 CS 핀 번호
  - 부작용: 내부 멤버 저장
*/
LAN::LAN(uint8_t cs_pin) : cs_pin(cs_pin) {}

/*
  begin()
  - Ethernet 스택 초기화와 MAC 기반 링크 시작을 수행한다.
  - 실패 시 false 반환.
*/
bool LAN::begin() {
  if (_ethernetMutex == NULL) {
    _ethernetMutex = xSemaphoreCreateMutex();
  }

  UIPEthernet.init(cs_pin);

  byte mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint64_t chipId = ESP.getEfuseMac();

  mac[1] = (chipId >> 32) & 0xFF;
  mac[2] = (chipId >> 24) & 0xFF;
  mac[3] = (chipId >> 16) & 0xFF;
  mac[4] = (chipId >> 8) & 0xFF;
  mac[5] = chipId & 0xFF;

  return UIPEthernet.begin(mac);
}

IPAddress LAN::getLocalIP() { return Ethernet.localIP(); }

/*
  resolveDomain(domain)
  - 입력: 도메인 또는 점표기 IP
  - 출력: 해석된 IP(실패 시 0.0.0.0)
  - 동시성: mutex 보호 구간에서 DNS 조회를 수행한다.
*/
IPAddress LAN::resolveDomain(String domain) {
  if (xSemaphoreTake(_ethernetMutex, portMAX_DELAY) != pdTRUE) {
    return IPAddress(0, 0, 0, 0);
  }

  IPAddress directIP;
  if (directIP.fromString(domain)) {
    xSemaphoreGive(_ethernetMutex);
    return directIP;
  }

  vTaskDelay(pdMS_TO_TICKS(100));

  DNSClient dns;
  IPAddress dnsIP = IPAddress(8, 8, 8, 8);;

  dns.begin(dnsIP);

  IPAddress out;
  int ok = dns.getHostByName(domain.c_str(), out);

  vTaskDelay(pdMS_TO_TICKS(100));

  xSemaphoreGive(_ethernetMutex);

  if (ok == 1) {
    return out;
  } else {
    return IPAddress(0, 0, 0, 0);
  }
}

/*
  isConnected()
  - 링크 상태 확인 + maintain 호출.
*/
bool LAN::isConnected() {
  if (xSemaphoreTake(_ethernetMutex, portMAX_DELAY) != pdTRUE) {
    return false;
  }

  vTaskDelay(pdMS_TO_TICKS(100));
  EthernetLinkStatus status = UIPEthernet.linkStatus();
  UIPEthernet.maintain();
  vTaskDelay(pdMS_TO_TICKS(100));

  xSemaphoreGive(_ethernetMutex);

  return status == LinkON;
}

/*
  withEthernet(callback)
  - mutex 보호 구간에서 EthernetClient를 생성해 콜백에 전달한다.
*/
bool LAN::withEthernet(std::function<bool(EthernetClient &)> callback) {
  if (xSemaphoreTake(_ethernetMutex, portMAX_DELAY) != pdTRUE) {
    return false;
  }

  EthernetClient client;

  vTaskDelay(pdMS_TO_TICKS(100));
  bool result = callback(client);
  vTaskDelay(pdMS_TO_TICKS(100));

  xSemaphoreGive(_ethernetMutex);

  return result;
}


