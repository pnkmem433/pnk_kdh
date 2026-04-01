#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <functional>
#include <vector>

/*
  ==================================================================================
  WifiManager 상세 설명
  ==================================================================================
  [문제 배경]
  현장 IoT 장치는 무선 환경이 불안정하다.
  - AP가 여러 개일 수 있고
  - 순간 끊김이 자주 발생하며
  - 부팅 직후 연결이 실패해도 장치가 멈추면 안 된다.

  [이 클래스의 역할]
  1) 여러 SSID 후보를 등록하고 연결 시도
  2) 연결/해제 이벤트를 애플리케이션(main.cpp)으로 전달
  3) 백그라운드에서 재연결을 계속 시도

  [설계 포인트]
  - begin()은 부팅 속도를 위해 제한 시간(10초) 내에서만 블로킹 시도
  - 그 이후에는 wifiTask가 비동기로 재연결 유지
  - ESP WiFi 이벤트 콜백은 정적 함수만 등록 가능하므로
    static -> instance 브릿지 패턴 사용
  ==================================================================================
*/

struct WifiInfo {
  String ssid;
  String password;
};

struct ConnectedInfo {
  String ssid;
  IPAddress ip;
};

class WifiManager {
public:
  // --------------------------------------------------------------------------
  // 생성자
  // - 연결 후보 SSID 목록을 주입한다.
  // - 목록 순서는 우선순위로 볼 수 있으나, 실제 선택은 신호/응답 상태에 좌우된다.
  // --------------------------------------------------------------------------
  explicit WifiManager(std::vector<WifiInfo> wifiInfos);

  // --------------------------------------------------------------------------
  // begin(name)
  // - Wi-Fi STA 모드 시작
  // - 등록된 AP로 초기 연결 시도
  // - 이벤트 핸들러 등록
  // - 재연결 태스크 시작
  // [입력]
  // - name: 비어있지 않으면 hostname으로 사용
  // [반환]
  // - true: API 수행 완료(연결 성공 여부와는 별개)
  // --------------------------------------------------------------------------
  bool begin(String name = "");

  // 현재 연결 여부 (WL_CONNECTED)
  bool isConnected();

  // DHCP로 받은 DNS 주소
  IPAddress dnsAddress();

  // 현재 RSSI(dBm). 연결 중이 아니면 의미 없는 값일 수 있다.
  int32_t rssi();

  // 연결 시 1회 이상 호출될 수 있는 콜백 등록
  void onConnect(std::function<void(ConnectedInfo)> cb);

  // 해제 시 호출될 수 있는 콜백 등록
  void onDisconnect(std::function<void()> cb);

  // 현재 연결 정보(SSID/IP) 스냅샷
  ConnectedInfo getConnectedInfo();

private:
  std::vector<WifiInfo> wifiInfos;
  WiFiMulti wifiMulti;

  std::function<void(ConnectedInfo)> connectCallback;
  std::function<void()> disconnectCallback;

  TaskHandle_t wifiTaskHandle = nullptr;

  // ESP 이벤트 API는 정적 함수 포인터만 받는다.
  // 따라서 static 핸들러에서 instance로 되돌려 전달한다.
  static WifiManager *instance;
  static void WiFiEventHandler(arduino_event_id_t event,
                               arduino_event_info_t info);
  void handleWiFiEvent(arduino_event_id_t event);

  // 주기적으로 Wi-Fi 연결 상태를 확인하는 백그라운드 태스크
  static void wifiTask(void *param);
};

#endif
