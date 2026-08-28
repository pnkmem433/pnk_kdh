import 'dart:async';

import 'package:flutter/material.dart';
import 'package:fxco_stylingbooth/core/theme/text_style.dart';
import 'package:fxco_stylingbooth/shared/http/session_repository.dart';
import 'package:fxco_stylingbooth/shared/http/system_parameter_repository.dart';
import 'package:fxco_stylingbooth/shared/models/dto/fitting_room_door_sensor_dto.dart';
import 'package:get/get.dart';

import 'feat/qr_display/qr_display_controller.dart';
import 'feat/waiting/waiting_controller.dart';
import 'shared/models/enum/screen_state.dart';

class AppStateController extends GetxController {
  var currentState = ScreenState.waiting.obs;
  final SystemParameterRepository systemParameterRepository = SystemParameterRepository();
  final SessionRepository sessionRepository = SessionRepository();

  int sessionSeq = (-1);
  Timer? _sessionTimer;
  FittingRoomDoorSensor? lastDoorState;
  RxInt time1 = 0.obs;

  @override
  void onInit() {
    super.onInit();
    initializeAppState();
    startSessionCheckLoop();
  }

  @override
  void onClose() {
    _sessionTimer?.cancel();
    super.onClose();
  }

  /// 앱 시작 시 호출
  Future<void> initializeAppState() async {
    time1.value = await systemParameterRepository.getSystemParameterBySeq(seq: 1) + 6;
  }

  /// 🔧 Waiting 상태로 전환 (lastDoorState 보존)
  void toWaiting() {
    print("AppStateController: Transitioning to waiting state");

    // 세션 관련 상태만 초기화 (lastDoorState는 보존)
    sessionSeq = (-1);
    // lastDoorState = null; // ❌ 제거: 이미 처리된 문 센서 데이터 중복 처리 방지

    // 상태 변경 (이것이 WaitingController의 ever 콜백을 트리거함)
    currentState.value = ScreenState.waiting;

    print("AppStateController: Waiting state transition completed - lastDoorState preserved");
  }

  /// QR Display 상태로 전환
  void toQRDisplay() {
    print("AppStateController: Transitioning to QR display state");
    currentState.value = ScreenState.qrDisplay;
  }

  /// 🔧 세션 상태 확인 루프 (에러 처리 강화)
  void startSessionCheckLoop() {
    _sessionTimer?.cancel(); // 중복 방지

    print("AppStateController: Starting session check loop");

    _sessionTimer = Timer.periodic(const Duration(seconds: 2), (timer) async {
      try {
        if (currentState.value != ScreenState.qrDisplay) {
          return; // QR Display 상태가 아니면 체크하지 않음
        }

        bool result = await sessionRepository.isSessionEnded();
        print("AppStateController: Session ended check result - $result");

        if (result) {
          print("AppStateController: Session ended - cleaning up and returning to waiting");

          // QRDisplayController 정리
          if (Get.isRegistered<QRDisplayController>()) {
            Get.delete<QRDisplayController>();
            print("AppStateController: QRDisplayController deleted");
          }

          // Waiting 상태로 복귀
          toWaiting();
        }
      } catch (e) {
        print("AppStateController: Error in session check loop - $e");
      }
    });
  }
}