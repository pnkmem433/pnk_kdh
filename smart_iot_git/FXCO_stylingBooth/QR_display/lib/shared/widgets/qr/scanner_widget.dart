import 'package:flutter/material.dart';
import 'package:fxco_stylingbooth/core/theme/layout_constants.dart';
import 'package:mobile_scanner/mobile_scanner.dart';

import 'scanner_widget_error.dart';

class ScannerWidget extends StatelessWidget {
  ScannerWidget({
  super.key,
  this.size = const Size(200, 200),
  required this.isCaptured,
  });

  BarcodeCapture? capture;
  final Size size;
  final Function(String) isCaptured;
  final MobileScannerController controller = MobileScannerController(
    facing: CameraFacing.front, // 전면 카메라 지정
  );

  String extractProductId(String url) {
    final uri = Uri.parse(url);
    return uri.queryParameters['product_id'] ?? '';
  }

  @override
  Widget build(BuildContext context) {
    return Builder(
      builder: (context) {
        return ClipRRect(
          borderRadius: AppLayout.radiusMd,
          child: SizedBox(
            width: size.width,
            height: size.height,
            child: MobileScanner(
              controller: controller,
              errorBuilder: (context, error, child) {
                return ScannerErrorWidget(error: error);
              },
              onDetect: (capture) {
                this.capture = capture;
                print(extractProductId(capture.barcodes.first.rawValue!));
                isCaptured(extractProductId(capture.barcodes.first.rawValue!));
              },
            ),
          ),
        );
      },
    );
  }
}