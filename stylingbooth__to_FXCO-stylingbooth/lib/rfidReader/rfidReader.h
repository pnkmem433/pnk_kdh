/*
  === 상세 주석 보강 블록 ===
  파일 역할:
  - RFID 리더 공통 인터페이스 정의.
*/
#ifndef RFID_READER_H
#define RFID_READER_H

#include "Arduino.h"
#include "functional"
#include <map>

/*
  RfidReader
  - 장치별 리더(FF701/FF704)를 동일 인터페이스로 사용하기 위한 추상 클래스.
  - 목적: 상위 로직(main)이 리더 모델 차이를 몰라도 동일 API로 제어하도록 하기 위함.
*/
class RfidReader {
public:
  /*
    begin()
    - 입력/출력: 없음
    - 부작용: 시리얼/하드웨어 초기화
    - 주의: startRead() 전에 반드시 호출
  */
  virtual void begin() = 0;

  /*
    startRead()
    - 입력/출력: 없음
    - 부작용: 리더 읽기 태스크 시작 또는 시작 명령 전송
    - 주의: 중복 호출 시 구현체가 내부적으로 태스크 중복 생성을 막아야 함
  */
  virtual void startRead() = 0;

  /*
    stopRead()
    - 입력/출력: 없음
    - 부작용: 읽기 태스크 중지 또는 중지 명령 전송
    - 주의: stop 후에도 누적 tags 맵은 clearTags() 전까지 남아있을 수 있음
  */
  virtual void stopRead() = 0;

  /*
    getTags()
    - 출력: 태그별 누적 카운트 맵
    - 주의: 구현체에 따라 복사본을 반환하므로 대용량일 때 비용이 생길 수 있음
  */
  virtual std::map<String, int> getTags() = 0;

  // 누적 태그 카운트 초기화
  virtual void clearTags() = 0;

  /*
    onRead(callback)
    - 입력: 태그 1건 수신 시 호출할 콜백
    - 부작용: 구현체의 읽기 태스크 컨텍스트에서 호출될 수 있음
    - 주의: 콜백에서 블로킹 작업을 길게 수행하면 수신율이 떨어질 수 있음
  */
  virtual void onRead(std::function<void(String)> callback) = 0;

  // 리더 모델명 문자열 반환(로그/디버깅용)
  virtual String readerName() = 0;
  virtual bool getPowerLevelDbm(int &outDbm) = 0;
  virtual bool setPowerLevelDbm(int dbm) = 0;
};

#include "fonkanFF701/fonkanFF701.h"
#include "fonkanFF704/fonkanFF704.h"

#endif


