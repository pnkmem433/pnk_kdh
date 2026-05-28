/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - Ethernet 접근 직렬화 API 정의.
*/
#ifndef LAN_H
#define LAN_H

#include "UIPEthernet.h"
#include "arduino.h"
#include "functional"
#include <mutex>
#include <Dns.h>

/*
  LAN
  - 목적: UIPEthernet 기반 유선 네트워크 접근을 직렬화해 안전하게 사용한다.
  - 보호 전략: _ethernetMutex로 DNS/링크점검/소켓 사용 구간을 보호한다.
  - 실패 시 동작: mutex 획득 실패 또는 DNS 실패 시 false/0.0.0.0을 반환한다.
*/
class LAN {
public:
  /*
    LAN(cs_pin)
    - 입력: ENC28J60 CS 핀
    - 출력: 객체 생성
    - 부작용: CS 핀 번호를 내부에 저장
  */
  LAN(uint8_t cs_pin);

  /*
    begin()
    - 입력/출력: 없음
    - 부작용: Ethernet 초기화 + MAC 설정 + 링크 시작
    - 실패/주의: 하드웨어/배선 문제 시 false를 반환하며 이후 통신 API가 실패할 수 있다.
  */
  bool begin();

  // 현재 로컬 IP 반환
  IPAddress getLocalIP();

  /*
    resolveDomain(domain)
    - 입력: 도메인 또는 점표기 IP 문자열
    - 출력: 해석된 IP 주소(실패 시 0.0.0.0)
    - 부작용: DNS 조회 시 mutex 구간을 점유
    - 주의: 도메인 직접 IP면 DNS를 생략하고 즉시 반환한다.
  */
  IPAddress resolveDomain(String domain);

  /*
    isConnected()
    - 입력/출력: 없음
    - 출력: 링크 활성 여부
    - 부작용: maintain 호출로 내부 상태 갱신
  */
  bool isConnected();

  /*
    withEthernet(callback)
    - 입력: EthernetClient를 사용하는 콜백
    - 출력: 콜백 반환값
    - 부작용: mutex로 해당 콜백 실행 구간을 직렬화
    - 주의: 콜백 내부에서 오래 블로킹하면 다른 네트워크 작업이 대기한다.
  */
  bool withEthernet(std::function<bool(EthernetClient &)> callback);

private:
  uint8_t cs_pin;
  SemaphoreHandle_t _ethernetMutex;
};

#endif


