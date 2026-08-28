import 'package:flutter/widgets.dart';

/// 위젯 간 간격과 둥글기 처리를 위한 상수 모음
class AppLayout {
  // Spacing (간격)
  static const double spaceEdge = 20.0;
  static const double spaceXs = 4.0;
  static const double spaceSm = 8.0;
  static const double spaceMd = 16.0;
  static const double spaceLg = 24.0;
  static const double spaceXl = 32.0;

  // SizedBox 편의 위젯
  static const Widget hSpaceXs = SizedBox(width: spaceXs);
  static const Widget hSpaceSm = SizedBox(width: spaceSm);
  static const Widget hSpaceMd = SizedBox(width: spaceMd);
  static const Widget hSpaceLg = SizedBox(width: spaceLg);
  static const Widget hSpaceXl = SizedBox(width: spaceXl);

  static const Widget vSpaceXs = SizedBox(height: spaceXs);
  static const Widget vSpaceSm = SizedBox(height: spaceSm);
  static const Widget vSpaceMd = SizedBox(height: spaceMd);
  static const Widget vSpaceLg = SizedBox(height: spaceLg);
  static const Widget vSpaceXl = SizedBox(height: spaceXl);

  // BorderRadius (둥글기)
  static const BorderRadius radiusXs = BorderRadius.all(Radius.circular(4.0));
  static const BorderRadius radiusSm = BorderRadius.all(Radius.circular(8.0));
  static const BorderRadius radiusMd = BorderRadius.all(Radius.circular(16.0));
  static const BorderRadius radiusLg = BorderRadius.all(Radius.circular(24.0));
  static const BorderRadius radiusXl = BorderRadius.all(Radius.circular(32.0));

  // Padding 편의값
  static const EdgeInsets paddingEdge = EdgeInsets.symmetric(horizontal: AppLayout.spaceEdge);
  static const EdgeInsets paddingSm = EdgeInsets.all(spaceSm);
  static const EdgeInsets paddingMd = EdgeInsets.all(spaceMd);
  static const EdgeInsets paddingLg = EdgeInsets.all(spaceLg);
  static const EdgeInsets paddingXl = EdgeInsets.all(spaceXl);
}
