import 'dart:io';
import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:mobile_scanner/mobile_scanner.dart';
import 'package:nfc_manager/nfc_manager.dart';
import '../../widgets/qr_scanner_dialog.dart';
import '../../widgets/nfc_reader_dialog.dart';
import '../../core/utils/snackbar_utils.dart';

class ScanService extends GetxService {
  
  /// Scans a QR code. Returns the code string or null if cancelled/failed.
  Future<String?> scanQr() async {
    if (Platform.isWindows) {
      return await _showMockDialog("QR Scan (Mock)", "Enter QR Data");
    }
    
    // Show full-screen QR scanner dialog
    final String? result = await Get.to<String>(
      () => const QrScannerDialog(),
      fullscreenDialog: true,
    );
    
    return result;
  }

  /// Reads NFC tag. Returns the UUID/Payload or null.
  Future<String?> readNfc() async {
    if (Platform.isWindows) {
      return await _showMockDialog("NFC Read (Mock)", "Enter NFC UUID");
    }

    // Check NFC availability
    bool isAvailable = await NfcManager.instance.isAvailable();
    if (!isAvailable) {
      SnackbarUtils.show("NFC 사용 불가", "이 기기는 NFC를 지원하지 않습니다.");
      return null;
    }

    // Show NFC reader dialog
    final String? result = await Get.dialog<String>(
      const NfcReaderDialog(),
      barrierDismissible: false,
    );
    
    return result;
  }

  Future<String?> _showMockDialog(String title, String hint) async {
    final textController = TextEditingController();
    return await Get.dialog<String>(
      AlertDialog(
        title: Text(title),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text("Simulating hardware scan on Windows."),
            const SizedBox(height: 16),
            TextField(
              controller: textController,
              decoration: InputDecoration(
                labelText: hint,
                border: const OutlineInputBorder(),
              ),
              autofocus: true,
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Get.back(),
            child: const Text("Cancel"),
          ),
          ElevatedButton(
            onPressed: () => Get.back(result: textController.text),
            child: const Text("Submit"),
          ),
        ],
      ),
    );
  }
}
