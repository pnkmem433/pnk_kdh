import 'package:flutter/material.dart';
import 'colors.dart';

/// 모바일 최적화된 텍스트 스타일 정의
class AppTextStyles {
  AppTextStyles._();

  static const _fontFamily = 'Pretendard';

  // 대제목: 앱 화면 최상단의 메인 타이틀에 어울립니다.
  static const TextStyle largeTitle = TextStyle(
    fontFamily: _fontFamily,
    fontSize: 52.0,
    fontWeight: FontWeight.w400,
    color: AppColors.adaptiveGrey700,
    height: 1.505,
//    height: 41,
  );

  // 대제목: 앱 화면 최상단의 메인 타이틀에 어울립니다.
  static const TextStyle mediumTitle = TextStyle(
    fontFamily: _fontFamily,
    fontSize: 40.0,
    fontWeight: FontWeight.w400,
    color: AppColors.adaptiveGrey700,
    height: 1.205,
//    height: 41,
  );
  // 대제목: 앱 화면 최상단의 메인 타이틀에 어울립니다.
  static const TextStyle smallTitle = TextStyle(
    fontFamily: _fontFamily,
    fontSize: 36.0,
    fontWeight: FontWeight.w400,
    color: AppColors.adaptiveGrey700,
    height: 1.333,
//    height: 41,
  );

  // 제목1: 주요 화면이나 섹션의 대표 제목으로 사용하기 좋습니다.
  static const TextStyle title1 = TextStyle(
    fontFamily: _fontFamily,
    fontSize: 28.0,
    fontWeight: FontWeight.w400,
    color: AppColors.adaptiveGrey700,
    height: 1.214,
//    height: 34,
  );

  // 제목2: 서브 섹션 제목이나 카드 타이틀에 적합합니다.
  static const TextStyle title2 = TextStyle(
    fontFamily: _fontFamily,
    fontSize: 22.0,
    fontWeight: FontWeight.w400,
    color: AppColors.adaptiveGrey700,
    height: 1.272,
//    height: 28,
  );

  // 제목3: 작은 섹션 제목이나 리스트 아이템 타이틀로 사용합니다.
  static const TextStyle title3 = TextStyle(
    fontFamily: _fontFamily,
    fontSize: 20.0,
    fontWeight: FontWeight.w400,
    color: AppColors.adaptiveGrey700,
    height: 1.25,
//    height: 25,
  );

  // 헤드라인: 중요 문구나 강조해야 할 텍스트에 적합합니다.
  static const TextStyle headline = TextStyle(
    fontFamily: _fontFamily,
    fontSize: 17.0,
    fontWeight: FontWeight.w600,
    color: AppColors.adaptiveGrey700,
    height: 1.294,
//    height: 22,
  );

  // 본문: 일반적인 본문 텍스트에 사용합니다.
  static const TextStyle body = TextStyle(
    fontFamily: _fontFamily,
    fontSize: 17.0,
    fontWeight: FontWeight.w400,
    color: AppColors.adaptiveGrey700,
    height: 1.294,
//    height: 22,
  );

  // 콜아웃: 짧은 강조 문장이나 호출 문구에 어울립니다.
  static const TextStyle callOut = TextStyle(
    fontFamily: _fontFamily,
    fontSize: 16.0,
    fontWeight: FontWeight.w400,
    color: AppColors.adaptiveGrey700,
    height: 1.312,
//    height: 21,
  );

  // 서브헤드: 본문의 부제목이나 작은 구역 제목에 적합합니다.
  static const TextStyle subhead = TextStyle(
    fontFamily: _fontFamily,
    fontSize: 15.0,
    fontWeight: FontWeight.w400,
    color: AppColors.adaptiveGrey700,
    height: 1.333,
//    height: 20,
  );

  // 풋노트: 각주나 부가 설명, 작은 보조 텍스트에 사용합니다.
  static const TextStyle footnote = TextStyle(
    fontFamily: _fontFamily,
    fontSize: 13.0,
    fontWeight: FontWeight.w400,
    color: AppColors.adaptiveGrey700,
    height: 1.384,
//    height: 18,
  );

  // 캡션1: 이미지 캡션이나 보조 설명 텍스트에 어울립니다.
  static const TextStyle caption1 = TextStyle(
    fontFamily: _fontFamily,
    fontSize: 12.0,
    fontWeight: FontWeight.w400,
    color: AppColors.adaptiveGrey700,
    height: 1.333,
//    height: 16,
  );

  // 캡션2: 아주 작은 보조 정보나 각종 레이블에 사용하기 좋습니다.
  static const TextStyle caption2 = TextStyle(
    fontFamily: _fontFamily,
    fontSize: 11.0,
    fontWeight: FontWeight.w400,
    color: AppColors.adaptiveGrey700,
    height: 1.181,
//    height: 13,
  );
}
