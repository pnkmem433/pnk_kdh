import 'package:flutter/cupertino.dart';
import 'package:qr_flutter/qr_flutter.dart';

class QrCode extends StatelessWidget {
  double size = 200.0;
  final String content;

  QrCode({
  super.key,
  required this.content,
  this.size = 200.0,
  });

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: size,
      height: size,
      child: QrImageView(
        data: content,
        gapless: true,
        padding: const EdgeInsets.all(4.0),
        errorStateBuilder: (cxt, err) {
          return const Center(
              child: Text("QR코드 생성 오류")
          );
        },
      ),
    );
  }
}
