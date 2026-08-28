import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'qr_scanner_controller.dart';

class QRScannerView extends StatelessWidget {
  const QRScannerView({super.key});

  @override
  Widget build(BuildContext context) {
    final controller = Get.put(QRScannerController());

    return Scaffold(
      body: Center(
        child: Obx(() {
          return controller.isLoading.value
              ? const CircularProgressIndicator()
              : Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              const Text(
                '대기 중입니다...',
                style: TextStyle(fontSize: 24),
              ),
              const SizedBox(height: 20),
              ElevatedButton(
                onPressed: controller.checkConditionAndNavigate,
                child: const Text('조건 체크 후 다음 화면으로'),
              ),
            ],
          );
        }),
      ),
    );
  }
}
