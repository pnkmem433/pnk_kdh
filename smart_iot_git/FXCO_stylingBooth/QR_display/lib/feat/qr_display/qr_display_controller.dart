import 'dart:async';

import 'package:fxco_stylingbooth/shared/models/dto/rfid_all_data_dto.dart';
import 'package:fxco_stylingbooth/shared/models/dto/rfid_scan_dto.dart';
import 'package:get/get.dart';

import '../../app_state_controller.dart';
import '../../shared/http/rfid_repository.dart';
import '../../shared/http/session_repository.dart';
import '../../shared/http/video_room_door_sensor_repository.dart';
import '../../shared/models/dto/video_room_door_sensor_dto.dart';

class QRDisplayController extends GetxController {
  final AppStateController appStateController = Get.find<AppStateController>();
  final RfidScanRepository rfidScanRepository = RfidScanRepository();
  final VideoRoomDoorSensorRepository videoRoomDoorSensorRepository = VideoRoomDoorSensorRepository();
  final SessionRepository sessionRepository = SessionRepository();

  Rxn<RfidScan> rfidScan = Rxn<RfidScan>();
  Rxn<RfidAllData> rfidAllData = Rxn<RfidAllData>();
  RxBool isRfidDataLoaded = false.obs;

  var isLoading = false.obs;
  var showNextStep = false.obs;
  bool isScanned = false;

  Timer? _doorCheckTimer;
  int _elapsedSeconds = 0;


  RxInt currentIndex = 0.obs;
  final List<String> images = [
    'assets/images/qr_step1.png',
    'assets/images/qr_step2.png',
    'assets/images/qr_step3.png',
  ];

  List<String> captions = [
    '1. 사용자 앱\n‘체험장 입장’ 터치',
    '2. ‘입장하기’ 터치\n ',
    '3. 왼쪽의 QR 스캔\n ',
  ];

  Map<String, String> newQRCode = {
    'IT00001': "CN11692",
    'IT00002': "CN11305",
    'IT00003': "CN11280",
    'IT00004': "CN11305",
    'IT00005': "CN11237",
    'IT00006': "CL002S",
  };

  Timer? _imageTimer;
  Timer? _rfidScanTimer;

  @override
  void onInit() {
    super.onInit();
    _initialFetch();    // 처음 한 번만 로딩 상태로 fetch
    startImageCycle();
    reloadRfidCycle();

    // 페이지 실행 시 문 상태 체크 시작
    startDoorCheckTimer();
  }

  @override
  void onReady() {
    super.onReady();
    fetchAllRfidScannedDataBySeq();
    startImageCycle();
    reloadRfidCycle();

    // 페이지 실행 시 문 상태 체크 시작
    startDoorCheckTimer();
  }

  @override
  void onClose() {
    _doorCheckTimer?.cancel();
    _doorCheckTimer = null;
    _imageTimer?.cancel();
    _rfidScanTimer?.cancel();
    _rfidScanTimer = null;
    super.onClose();
  }

  void _initialFetch() async {
    isRfidDataLoaded.value = false;
    fetchAllRfidScannedDataBySeq();
    isRfidDataLoaded.value = true;
  }

  void startImageCycle() {
    _imageTimer?.cancel();
    _imageTimer = Timer.periodic(const Duration(seconds: 5), (timer) {
      currentIndex.value = (currentIndex.value + 1) % images.length;
    });
  }

  void reloadRfidCycle() {
    _rfidScanTimer?.cancel();
    _rfidScanTimer = Timer.periodic(const Duration(seconds: 3), (timer) {
      fetchAllRfidScannedDataBySeq();
    });
  }

//  Future<void> checkIsDoorOpened() async {
//    bool isOpened = await videoRoomDoorSensorRepository.readRfidScannedBySeq(sessionSeq: appStateController.sessionSeq);
//    if (!isOpened) {
//      checkConditionAndNavigate();
//    }
//  }

  void startDoorCheckTimer() {
    // 기존 타이머 정리
    _doorCheckTimer?.cancel();

    _doorCheckTimer = Timer.periodic(const Duration(seconds: 2), (timer) async {
      print("startDoorCheckTimer: ${DateTime.now()}");
      _elapsedSeconds += 2;

      // 문 열림 여부 확인
      VideoRoomDoorSensorLast videoRoomDoorSensorLast = await videoRoomDoorSensorRepository.readRfidScannedBySeq(sessionSeq: appStateController.sessionSeq);
      if (videoRoomDoorSensorLast.isOpened == 1) {
        /// 여기에 RFID와 비교하는 함수

        if (rfidAllData.value != null && videoRoomDoorSensorLast.eventTime.isAfter(rfidAllData.value!.scannedAt)) {
          timer.cancel();
          checkConditionAndNavigate();
          return;
        }
      }

//      // 5분(300초) 경과 시 강제 실행
//      if (_elapsedSeconds >= appStateController.time2.value) {
//        timer.cancel();
//        checkConditionAndNavigate();
//      }
    });
  }

  Future<void> checkConditionAndNavigate() async {
    print("appStateController.toWaiting();");
    appStateController.toWaiting();
  }

//  void fetchScannedCloth() async {
//    rfidScan.value = await rfidScanRepository.readRfidScannedBySeq(
//        sessionSeq: appStateController.sessionSeq);
//  }

  void fetchAllRfidScannedDataBySeq() async {
//    isRfidDataLoaded.value = false;

    try {
      // 데이터 요청
      final result = await rfidScanRepository.readAllRfidScannedDataBySeq(
        sessionSeq: appStateController.sessionSeq,
      );

      // 결과가 null이 아니면 업데이트 및 updateIsScannedSession 실행
      if (result != null) {
        rfidAllData.value = result;
        await sessionRepository.updateIsScannedSession(
          sessionSeq: appStateController.sessionSeq,
        );
      }
    } catch (e) {
      // 에러 발생 시 무시 (updateIsScannedSession 실행하지 않음)
      print("readAllRfidScannedDataBySeq 오류: $e");
    } /*finally {
      isRfidDataLoaded.value = true; // 성공/실패 관계 없이 완료됨을 알림
    }*/
  }

  void createRfidScan(String qrCode) async {
    isScanned = true;
    if (isScanned) {
      bool result = await rfidScanRepository.createRfidScan(
          qrCode: qrCode, sessionSeq: appStateController.sessionSeq);
      print("createRfidScan $qrCode 추가 ${result ? "성공" : "실패"}");
      if (result) {
        Get.back();
        fetchAllRfidScannedDataBySeq();
      } else {
        isScanned = false;
      }
    }

  }

  void clickNextStep() => showNextStep.value = true;
  void clickNextStepEnd() => showNextStep.value = false;
}
