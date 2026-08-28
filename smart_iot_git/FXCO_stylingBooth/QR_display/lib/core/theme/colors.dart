import 'package:flutter/material.dart';

/// 앱 전역에서 사용되는 색상 정의
class AppColors {
  AppColors._();

  static const Color white   = Color(0xFFffffff);

  // Background / Surface
  static const Color adaptiveGreyBackground = adaptiveGrey100;
  static const Color adaptiveBackground     = white;

  // Grey
  static const Color adaptiveGrey50   = Color(0xFFf9fafb);
  static const Color adaptiveGrey100  = Color(0xFFf2f4f6);
  static const Color adaptiveGrey200  = Color(0xFFe5e8eb);
  static const Color adaptiveGrey300  = Color(0xFFd1d6db);
  static const Color adaptiveGrey400  = Color(0xFFb0b8c1);
  static const Color adaptiveGrey500  = Color(0xFF8b95a1);
  static const Color adaptiveGrey600  = Color(0xFF6b7684);
  static const Color adaptiveGrey700  = Color(0xFF4e5968);
  static const Color adaptiveGrey800  = Color(0xFF333d4b);
  static const Color adaptiveGrey900  = Color(0xFF191f28);

  // Primary
  static const Color adaptivePrimary50  = adaptivePink50;
  static const Color adaptivePrimary100 = adaptivePink100;
  static const Color adaptivePrimary200 = adaptivePink200;
  static const Color adaptivePrimary300 = adaptivePink300;
  static const Color adaptivePrimary400 = adaptivePink400;
  static const Color adaptivePrimary500 = adaptivePink500;
  static const Color adaptivePrimary600 = adaptivePink600;
  static const Color adaptivePrimary700 = adaptivePink700;
  static const Color adaptivePrimary800 = adaptivePink800;
  static const Color adaptivePrimary900 = adaptivePink900;

  // Green
  static const Color adaptiveGreen50   = Color(0xFFf0faf6);
  static const Color adaptiveGreen100  = Color(0xFFaeefd5);
  static const Color adaptiveGreen200  = Color(0xFF76e4b8);
  static const Color adaptiveGreen300  = Color(0xFF3fd599);
  static const Color adaptiveGreen400  = Color(0xFF15c47e);
  static const Color adaptiveGreen500  = Color(0xFF03b26c);
  static const Color adaptiveGreen600  = Color(0xFF02a262);
  static const Color adaptiveGreen700  = Color(0xFF029359);
  static const Color adaptiveGreen800  = Color(0xFF028450);
  static const Color adaptiveGreen900  = Color(0xFF027648);

  // Blue
  static const Color adaptiveBlue50    = Color(0xFFe8f3ff);
  static const Color adaptiveBlue100   = Color(0xFFc9e2ff);
  static const Color adaptiveBlue200   = Color(0xFF90c2ff);
  static const Color adaptiveBlue300   = Color(0xFF64a8ff);
  static const Color adaptiveBlue400   = Color(0xFF4593fc);
  static const Color adaptiveBlue500   = Color(0xFF3182f6);
  static const Color adaptiveBlue600   = Color(0xFF2272eb);
  static const Color adaptiveBlue700   = Color(0xFF1b64da);
  static const Color adaptiveBlue800   = Color(0xFF1957c2);
  static const Color adaptiveBlue900   = Color(0xFF194aa6);

  // Red
  static const Color adaptiveRed50     = Color(0xFFffeeee);
  static const Color adaptiveRed100    = Color(0xFFffd4d6);
  static const Color adaptiveRed200    = Color(0xFFfeafb4);
  static const Color adaptiveRed300    = Color(0xFFfb8890);
  static const Color adaptiveRed400    = Color(0xFFf66570);
  static const Color adaptiveRed500    = Color(0xFFf04452);
  static const Color adaptiveRed600    = Color(0xFFe42939);
  static const Color adaptiveRed700    = Color(0xFFd22030);
  static const Color adaptiveRed800    = Color(0xFFbc1b2a);
  static const Color adaptiveRed900    = Color(0xFFa51926);

  // Pink
  static const Color adaptivePink50    = Color(0xffFFEEF4);
  static const Color adaptivePink100   = Color(0xffFFD4E4);
  static const Color adaptivePink200   = Color(0xffFEAFCF);
  static const Color adaptivePink300   = Color(0xffFB88B6);
  static const Color adaptivePink400   = Color(0xffF665A1);
  static const Color adaptivePink500   = Color(0xffF0448C);
  static const Color adaptivePink600   = Color(0xffE42977);
  static const Color adaptivePink700   = Color(0xffD2206A);
  static const Color adaptivePink800   = Color(0xffBC1B61);
  static const Color adaptivePink900   = Color(0xffA51956);

  // Status
  static const Color success           = adaptiveGreen700;
  static const Color warning           = Color(0xFFF57C00);
  static const Color error             = adaptiveRed700;
}