import 'package:get/get.dart';

import '../../app_state_controller.dart';

class QRScannerController extends GetxController {
  final AppStateController appStateController = Get.find<AppStateController>();

  // 대기 화면에서 서버 상태나 조건을 주기적으로 체크
  var isLoading = false.obs;

  @override
  void onInit() {
    super.onInit();
    // 예시: 5초마다 조건 체크
    ever(isLoading, (_) {
      // 필요시 상태 변화 시 동작 추가
    });
  }

  Future<void> checkConditionAndNavigate() async {
    isLoading.value = true;
    try {
      // TODO: 서버 호출 혹은 조건 검사 로직 추가
      await Future.delayed(const Duration(seconds: 2));

      // 조건 만족 시 QR 화면으로 이동
      appStateController.toQRDisplay();
    } finally {
      isLoading.value = false;
    }
  }
}
