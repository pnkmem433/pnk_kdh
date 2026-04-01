#ifndef CLOTH_DATA_H
#define CLOTH_DATA_H

#include <Arduino.h>
#include <vector>

/*
  ==================================================================================
  ClothData 모델 상세 설명
  ==================================================================================
  백엔드 JSON을 UI 렌더링 가능한 내부 데이터 구조로 변환한다.
  주요 필드:
  - price: 가격
  - webSite: QR에 넣을 URL
  - sizeColorOptions: 사이즈별 색상 목록

  주의:
  - 색상은 JSON에서 보통 HEX 문자열(#RRGGBB)로 전달되며
    내부에서는 TFT 친화적인 RGB565로 변환해 보관한다.
  ==================================================================================
*/

struct SizeColorOption {
  int size;                     // 예: 55
  std::vector<uint32_t> colors; // RGB565 색상값 목록
};

struct ClothData {
  int price;       // 예: 850000
  String webSite;  // 예: https://...
  std::vector<SizeColorOption> sizeColorOptions;

  // JSON 문자열을 ClothData로 파싱
  static ClothData fromString(const String &json);
};

#endif
