import 'package:get/get.dart';
import '../../data/services/api_service.dart';
import '../../data/services/scan_service.dart';
import '../../core/utils/snackbar_utils.dart';

class SmartRackController extends GetxController {
  final ApiService _api = Get.find<ApiService>();
  final ScanService _scan = Get.find<ScanService>();
  
  final smartRackId = RxnString();
  final selectedPosition = RxnInt();
  
  final isSmartRackScanning = false.obs;
  
  bool get isSmartRackRecognized => smartRackId.value != null;
  bool get isPositionSelected => selectedPosition.value != null;
  bool get canSave => isSmartRackRecognized && isPositionSelected;
  
  String? get canonicalSmartRackId => 
      smartRackId.value != null ? _toCanonicalHex(smartRackId.value!) : null;
  
  final isSaving = false.obs;

  Future<void> scanSmartRack() async {
    isSmartRackScanning.value = true;
    try {
      final result = await _scan.readNfc();
      if (result != null && result.isNotEmpty) {
        smartRackId.value = result;
      }
    } finally {
      isSmartRackScanning.value = false;
    }
  }
  
  void selectPosition(int position) {
    selectedPosition.value = position;
  }

  String _toCanonicalHex(String byteDelimitedHex) {
    // Keep UI as byte-delimited hex (e.g., "AA:BB:CC"), but send canonical hex ("AABBCC").
    final stripped = byteDelimitedHex.replaceAll(RegExp(r'[^0-9A-Fa-f]'), '');
    return stripped.toUpperCase();
  }

  Future<void> savePair() async {
    if (!canSave) return;
    
    isSaving.value = true;
    final canonicalHangerlegUuid = _toCanonicalHex(smartRackId.value!);
    final success = await _api.replaceHangerLeg(
      hangerlegUuid: canonicalHangerlegUuid,
      rackSeq: 1,
      position: selectedPosition.value!-1,
    );
    isSaving.value = false;
    
    if (success) {
      SnackbarUtils.show("Success", "헹거랙 페어링이 저장되었습니다.");
      smartRackId.value = null;
      selectedPosition.value = null;
    } else {
      SnackbarUtils.show("Error", "저장에 실패했습니다.");
    }
  }
}
