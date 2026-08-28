import 'dart:async';

import 'package:fxco_stylingbooth/shared/http/fitting_room_door_sensor_repository.dart';
import 'package:fxco_stylingbooth/shared/models/dto/fitting_room_door_sensor_dto.dart';
import 'package:get/get.dart';
import 'package:video_player/video_player.dart';

import '../../app_state_controller.dart';
import '../../shared/http/rfid_repository.dart';
import '../../shared/http/session_repository.dart';
import '../../shared/models/enum/screen_state.dart';

class WaitingController extends GetxController {
  final AppStateController appStateController = Get.find<AppStateController>();
  final SessionRepository sessionRepository = SessionRepository();
  final FittingRoomDoorSensorRepository fittingRoomDoorSensorRepository =
  FittingRoomDoorSensorRepository();

  // UI 상태
  final RxBool isScanned = false.obs;
  final RxBool showAlertMessage = false.obs;
  final RxInt countdown = 10.obs;
  final RxBool isCountdownRunning = false.obs;

  Timer? _doorCheckTimer;
  Timer? _countdownTimer;
  bool _isPollingTickRunning = false;
  bool _isDisposed = false;

  // 비디오 관련 상태
  late final VideoPlayerController videoPlayerController;
  final RxBool isVideoReady = false.obs;
  final RxBool hasVideoError = false.obs; // 🔧 에러 상태 추가

  // 🔧 비디오 초기화 개선
  Future<void> _initVideoPlayer() async {
    try {
      print("WaitingController: Initializing video player");

      videoPlayerController = VideoPlayerController.asset('assets/videos/leaf.mp4');

      // 초기화 완료까지 대기
      await videoPlayerController.initialize();

      if (_isDisposed) {
        print("WaitingController: Controller disposed during video init");
        return;
      }

      // 초기화 성공 후 설정 적용
      await videoPlayerController.setLooping(true);
      await videoPlayerController.setPlaybackSpeed(0.75);

      // 재생 시작
      await videoPlayerController.play();

      print("WaitingController: Video player initialized and playing");
      isVideoReady.value = true;
      hasVideoError.value = false;

    } catch (e) {
      print("WaitingController: Video initialization failed - $e");
      hasVideoError.value = true;
      isVideoReady.value = true; // UI는 표시하되 에러 상태로
    }
  }

  // 🔧 비디오 상태 확인 및 복구
  void _checkAndRestoreVideo() {
    if (_isDisposed || hasVideoError.value) return;

    try {
      // 비디오가 멈춤 상태면 다시 재생
      if (videoPlayerController.value.isInitialized && !videoPlayerController.value.isPlaying) {
        print("WaitingController: Restoring video playback");
        videoPlayerController.play();
      }
    } catch (e) {
      print("WaitingController: Error restoring video - $e");
    }
  }

  @override
  void onInit() {
    super.onInit();
    _isDisposed = false;

    // 🔧 비동기 초기화로 변경
    _initVideoPlayer();

    // waiting 진입 때마다 안전 초기화
    ever(appStateController.currentState, (st) {
      if (_isDisposed) return;

      if (st == ScreenState.waiting) {
        _fullReset();
        startDoorPolling();
        // 🔧 비디오 상태 복구 시도
        _checkAndRestoreVideo();
      } else {
        stopAllTimers();
        // 🔧 다른 화면으로 전환 시 비디오 일시정지
        _pauseVideo();
      }
    });

    // 초기 상태가 waiting이면 시작
    if (appStateController.currentState.value == ScreenState.waiting) {
      _fullReset();
      startDoorPolling();
    }
  }

  // 🔧 비디오 일시정지
  void _pauseVideo() {
    try {
      if (videoPlayerController.value.isInitialized && videoPlayerController.value.isPlaying) {
        videoPlayerController.pause();
        print("WaitingController: Video paused");
      }
    } catch (e) {
      print("WaitingController: Error pausing video - $e");
    }
  }

  // 🔧 비디오 재생 재개
  void _resumeVideo() {
    try {
      if (videoPlayerController.value.isInitialized && !videoPlayerController.value.isPlaying) {
        videoPlayerController.play();
        print("WaitingController: Video resumed");
      }
    } catch (e) {
      print("WaitingController: Error resuming video - $e");
    }
  }

  void _fullReset() {
    print("WaitingController: UI reset initiated");
    stopAllTimers();

    isScanned.value = false;
    showAlertMessage.value = false;
    isCountdownRunning.value = false;
    countdown.value = 10;
    _isPollingTickRunning = false;

    // 🔧 비디오 재생 상태 복구
    _resumeVideo();

    print("WaitingController: UI reset completed");
  }

  void stopAllTimers() {
    print("WaitingController: Stopping all timers");

    _doorCheckTimer?.cancel();
    _doorCheckTimer = null;

    _countdownTimer?.cancel();
    _countdownTimer = null;

    _isPollingTickRunning = false;
  }

  @override
  void onClose() {
    print("WaitingController: onClose called");
    _isDisposed = true;

    // 🔧 비디오 리소스 정리 순서 개선
    stopAllTimers();

    try {
      if (videoPlayerController.value.isInitialized) {
        videoPlayerController.pause();
      }
      videoPlayerController.dispose();
    } catch (e) {
      print("WaitingController: Error disposing video player - $e");
    }

    super.onClose();
  }

  void forceStopCountdown() {
    print("WaitingController: Force stopping countdown");
    _countdownTimer?.cancel();
    _countdownTimer = null;
    isCountdownRunning.value = false;
    isScanned.value = false;
  }

  void startDoorPolling() {
    if (_isDisposed || _doorCheckTimer?.isActive == true) {
      print("WaitingController: Door polling already active or disposed");
      return;
    }

    print("WaitingController: Starting door polling");

    _doorCheckTimer = Timer.periodic(const Duration(seconds: 1), (timer) async {
      if (_isDisposed || _isPollingTickRunning) return;
      if (appStateController.currentState.value != ScreenState.waiting) {
        print("WaitingController: Not in waiting state, stopping polling");
        return;
      }

      _isPollingTickRunning = true;

      try {
        final newState = await fittingRoomDoorSensorRepository
            .readFittingRoomDoorSensorLatestOne();

        if (newState == null) {
          print("WaitingController: No door state received");
          return;
        }

        // 같은 이벤트면 무시
        if (appStateController.lastDoorState != null &&
            _isSameDoorState(newState, appStateController.lastDoorState!)) {
          return;
        }

        // 새 상태 반영
        appStateController.lastDoorState = newState;
        print("WaitingController: New door state - isOpened: ${newState.isOpened}, seq: ${newState.seq}");

        if (newState.isOpened == 1) {
          print("WaitingController: Door opened - stopping current attempt");
          _handleDoorOpened();
        } else if (newState.isOpened == 0) {
          print("WaitingController: Door closed - starting new attempt");
          _handleDoorClosed();
        }

      } catch (e) {
        print("WaitingController: Error in door polling - $e");
      } finally {
        _isPollingTickRunning = false;
      }
    });
  }

  void _handleDoorOpened() {
    print("WaitingController: Handling door opened");
    forceStopCountdown();
    isScanned.value = false;
    showAlertMessage.value = false;
    countdown.value = 10;
  }

  void _handleDoorClosed() {
    print("WaitingController: Handling door closed");

    if (isCountdownRunning.value) {
      print("WaitingController: Countdown already running, ignoring");
      return;
    }

    showAlertMessage.value = false;
    isScanned.value = true;
    countdown.value = 10;
    _startCountdownProcess();
  }

  void _startCountdownProcess() {
    if (_isDisposed || isCountdownRunning.value) {
      print("WaitingController: Cannot start countdown - disposed: $_isDisposed, running: ${isCountdownRunning.value}");
      return;
    }

    print("WaitingController: Starting countdown process");
    isCountdownRunning.value = true;
    countdown.value = 10;

    _countdownTimer = Timer.periodic(const Duration(seconds: 1), (timer) async {
      if (_isDisposed || appStateController.currentState.value != ScreenState.waiting) {
        print("WaitingController: Countdown stopped - disposed: $_isDisposed, state: ${appStateController.currentState.value}");
        _stopCountdownTimer();
        return;
      }

      if (appStateController.lastDoorState?.isOpened == 1) {
        print("WaitingController: Door opened during countdown - aborting");
        _stopCountdownTimer();
        return;
      }

      countdown.value--;
      print("WaitingController: Countdown - ${countdown.value}");

      if (countdown.value <= 0) {
        print("WaitingController: Countdown completed - checking session");
        _stopCountdownTimer();
        await _checkSessionAndProceed();
      }
    });
  }

  void _stopCountdownTimer() {
    print("WaitingController: Stopping countdown timer");
    _countdownTimer?.cancel();
    _countdownTimer = null;
    isCountdownRunning.value = false;
    isScanned.value = false;
  }

  Future<void> _checkSessionAndProceed() async {
    if (_isDisposed) return;

    try {
      print("WaitingController: Checking session untracked status");
      final untracked = await sessionRepository.readSessionUntracked();
      print("WaitingController: Session untracked result - $untracked");

      if (untracked == false) {
        print("WaitingController: Session tracked - getting last session");
        final result = await sessionRepository.getLastSession();

        if (result != null) {
          print("WaitingController: Last session found - $result");
          appStateController.sessionSeq = result;
          showAlertMessage.value = false;

          await Future.delayed(const Duration(seconds: 2));
          if (!_isDisposed && appStateController.currentState.value == ScreenState.waiting) {
            print("WaitingController: Transitioning to QR display");
            appStateController.toQRDisplay();
          }
        } else {
          print("WaitingController: No last session found - showing alert");
          showAlertMessage.value = true;
        }
      } else {
        print("WaitingController: Session untracked - showing alert");
        showAlertMessage.value = true;
      }
    } catch (e) {
      print("WaitingController: Error in session check - $e");
      showAlertMessage.value = true;
    }
  }

  bool _isSameDoorState(FittingRoomDoorSensor a, FittingRoomDoorSensor b) {
    if (a.seq != null && b.seq != null) {
      bool isSame = a.seq == b.seq;
      print("WaitingController: Comparing by seq - ${a.seq} vs ${b.seq} = $isSame");
      return isSame;
    }

    bool isSame = a.isOpened == b.isOpened && a.eventTime == b.eventTime;
    print("WaitingController: Comparing by isOpened+eventTime - ${a.isOpened}/${a.eventTime} vs ${b.isOpened}/${b.eventTime} = $isSame");
    return isSame;
  }
}