import 'package:flutter/material.dart';
import 'package:fxco_stylingbooth/core/theme/text_style.dart';
import 'package:get/get.dart';

import 'app_state_controller.dart';
import 'feat/qr_display/qr_display_controller.dart';
import 'feat/qr_display/qr_display_view.dart';
import 'feat/qr_scanner/qr_scanner_view.dart';
import 'feat/waiting/waiting_controller.dart';
import 'feat/waiting/waiting_view.dart';
import 'shared/models/enum/screen_state.dart';

class AppRoot extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    final controller = Get.find<AppStateController>();

    return Obx(() {
      final screen = controller.currentState.value;

      // controller 생성/삭제
      if (screen == ScreenState.waiting) {
        Get.delete<QRDisplayController>();
        Get.put(WaitingController());
      } else if (screen == ScreenState.qrDisplay) {
        Get.delete<WaitingController>();
        Get.put(QRDisplayController());
      }

      return AnimatedSwitcher(
        duration: const Duration(milliseconds: 500),
        switchInCurve: Curves.easeIn,
        switchOutCurve: Curves.easeOut,
        child: _buildPage(screen),
      );
    });
  }

  Widget _buildPage(ScreenState screen) {
    switch (screen) {
      case ScreenState.waiting:
        return const WaitingView(key: ValueKey('waiting'));
      case ScreenState.qrDisplay:
        return const QRDisplayView(key: ValueKey('qrDisplay'));
    }
  }
}
