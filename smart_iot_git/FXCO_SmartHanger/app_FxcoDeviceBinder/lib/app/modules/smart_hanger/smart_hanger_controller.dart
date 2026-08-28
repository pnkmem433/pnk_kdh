import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import '../../data/services/api_service.dart';
import '../../data/services/mqtt_service.dart';
import '../../data/services/scan_service.dart';
import '../../core/utils/snackbar_utils.dart';

enum HangerStep { scan, select, listen }

class SmartHangerController extends GetxController {
  final ApiService _api = Get.find<ApiService>();
  final MqttService _mqtt = Get.find<MqttService>();
  final ScanService _scan = Get.find<ScanService>();

  final currentStep = HangerStep.scan.obs;
  final scannedQr = ''.obs;
  final statusMessage = 'QR코드를 스캔해주세요'.obs;
  final expectedTopic = ''.obs;
  final isConnecting = false.obs;
  final isClothesMatched = RxnBool();
  
  final clothesList = <String>[].obs; // Just IDs or Names for mock
  final selectedClothesIndex = (-1).obs;
  
  // Event logs
  final eventLog = <String>[].obs;

  // Expose MQTT status for UI
  RxBool get mqttConnected => _mqtt.isConnected;
  RxBool get mqttSubscribed => _mqtt.isSubscribed;
  RxString get mqttSubscribedTopic => _mqtt.subscribedTopic;
  RxString get mqttConnectionStateLabel => _mqtt.connectionStateLabel;
  RxString get mqttConnectionReturnCodeLabel => _mqtt.connectionReturnCodeLabel;

  String get mqttTopicForDisplay {
    if (mqttSubscribedTopic.value.isNotEmpty) return mqttSubscribedTopic.value;
    return expectedTopic.value;
  }

  @override
  void onInit() {
    super.onInit();
    // Mock clothes data
    clothesList.addAll(List.generate(14, (index) => 'Clothes Item #${index + 1}'));
    
    // Listen to MQTT globally, but filter in logic
    _mqtt.eventStream.listen(_handleMqttEvent);
  }

  // MQTT Configuration
  final String mqttBroker = "192.168.1.67"; // Host/IP only
  final String mqttUsername = "pnks"; // TODO: Update
  final String mqttPassword = "pnks1111"; // TODO: Update
  final int mqttPort = 1883;
  static const String _fixedMqttTopic =
      "smart_hanger/event/8E4EF94C-6A44-4F70-6E14-BC410EB0F021";

  Future<void> startScan() async {
    var result = await _scan.scanQr();
    if (result != null && result.isNotEmpty) {
      result = result.trim(); // Trim whitespace/newlines
      statusMessage.value = "QR 확인 중...";
      final isValid = await _api.checkHanger(result);
      if (isValid) {
        scannedQr.value = result;
        currentStep.value = HangerStep.select;
        statusMessage.value = "연결할 옷을 선택해주세요";
        
        // Connect to MQTT and Subscribe
        await _connectAndSubscribe(result);
        
      } else {
        SnackbarUtils.show("Error", "유효하지 않은 QR코드입니다.");
        statusMessage.value = "다시 스캔해주세요";
      }
    }
  }

  Future<void> _connectAndSubscribe(String qrCode) async {
    isConnecting.value = true;
    try {
      final success = await _mqtt.connect(
        mqttBroker,
        mqttUsername,
        mqttPassword,
        port: mqttPort,
      );
      if (success) {
        final commandTopic = "smart_hanger/command/$qrCode";
        final eventTopic = "smart_hanger/event/$qrCode";
        
        expectedTopic.value = eventTopic;

        // Command 토픽 구독
        _mqtt.subscribe(commandTopic);
        final commandSubscribed = await _waitForSubscription(commandTopic);
        
        // Event 토픽 구독
        _mqtt.subscribe(eventTopic);
        final eventSubscribed = await _waitForSubscription(eventTopic);
        
        final results = [
          "command ${commandSubscribed ? 'Success' : 'Failure'}",
          "event ${eventSubscribed ? 'Success' : 'Failure'}",
        ];
        
        if (!commandSubscribed || !eventSubscribed) {
          _showMqttSnack(
            success: false, 
            message: results.join("\n")
          );
        }
      } else {
        _showMqttSnack(success: false, message: "MQTT 브로커 연결 실패");
      }
    } finally {
      isConnecting.value = false;
    }
  }

  Future<void> subscribeFixedTopic() async {
    isConnecting.value = true;
    try {
      expectedTopic.value = _fixedMqttTopic;
      var connected = _mqtt.isConnected.value;
      if (!connected) {
        connected = await _mqtt.connect(
          mqttBroker,
          mqttUsername,
          mqttPassword,
          port: mqttPort,
        );
      }

      if (!connected) {
        _showMqttSnack(success: false, message: "MQTT 연결 실패");
        return;
      }

      _mqtt.subscribe(_fixedMqttTopic);
      final subscribed = await _waitForSubscription(_fixedMqttTopic);
      if (!subscribed) {
        _showMqttSnack(success: false, message: "MQTT 구독 실패");
      }
    } finally {
      isConnecting.value = false;
    }
  }

  void selectClothes(int index) {
    selectedClothesIndex.value = index;
  }

  Future<void> saveAssociation() async {
    if (selectedClothesIndex.value == -1) return;
    
    final selectedIndex = selectedClothesIndex.value;
    final clothesSeq = selectedIndex + 1;
    statusMessage.value = "저장 중...";
    
    final statusCode = await _api.matchClothes(
      hangerUuid: scannedQr.value,
      clothesSeq: clothesSeq,
    );
    if (statusCode == 200) {
      isClothesMatched.value = true;
      final commandTopic = "smart_hanger/command/${scannedQr.value}";
      _mqtt.publishJson(commandTopic, const {"action": "restart"});
      currentStep.value = HangerStep.listen;
      // statusMessage.value = "이벤트 대기 중...\nTopic: smart_hanger/event/${scannedQr.value}";
      statusMessage.value = "픽다운 대기 중...";
    } else {
      isClothesMatched.value = false;
      final codeLabel = statusCode?.toString() ?? "no-response";
      SnackbarUtils.show("오류", "저장 실패: HTTP $codeLabel");
    }
  }

  void _handleMqttEvent(Map<String, dynamic> payload) async {
    if (currentStep.value != HangerStep.listen) return;

    if (payload['event'] == 'pickdown' && payload.containsKey('card')) {
      final hangerLegUuid = payload['card'];
      eventLog.add("Event Received: Pickdown from $hangerLegUuid");
      
      final success = await _api.replaceHanger(
        hangerUuid: scannedQr.value,
        hangerlegUuid: hangerLegUuid,
      );
      
      if (success) {
         SnackbarUtils.show("Success", "헹거랙과 연결 성공");
         eventLog.add("API Sent Success");
         reset();
      } else {
         SnackbarUtils.show("Error", "Failed to replace hanger.");
         eventLog.add("API Sent Failure");
      }
    }
  }
  
  // Debug helper to mock incoming MQTT
  void debugSimulateMqtt() {
    _mqtt.simulateEvent("smart_hanger/event/${scannedQr.value}", {
      "event": "pickdown",
      "card": "LEG-UUID-MOCK-1234"
    });
  }
  
  void reset() {
    _mqtt.disconnect();
    currentStep.value = HangerStep.scan;
    scannedQr.value = '';
    selectedClothesIndex.value = -1;
    eventLog.clear();
    expectedTopic.value = '';
    isClothesMatched.value = null;
    statusMessage.value = 'QR코드를 스캔해주세요';
  }

  Future<bool> _waitForSubscription(
    String topic, {
    Duration timeout = const Duration(seconds: 3),
  }) async {
    final start = DateTime.now();
    while (DateTime.now().difference(start) < timeout) {
      if (_mqtt.isSubscribed.value && _mqtt.subscribedTopic.value == topic) {
        return true;
      }
      await Future.delayed(const Duration(milliseconds: 100));
    }
    return false;
  }

  void _showMqttSnack({required bool success, required String message}) {
    SnackbarUtils.show(
      success ? "Success" : "Error",
      message,
    );
  }
}
