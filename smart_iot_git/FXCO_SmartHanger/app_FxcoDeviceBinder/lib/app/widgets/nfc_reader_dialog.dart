import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'package:nfc_manager/nfc_manager.dart';
import '../theme/app_colors.dart';
import '../theme/app_text_styles.dart';
import '../core/utils/snackbar_utils.dart';

class NfcReaderDialog extends StatefulWidget {
  const NfcReaderDialog({super.key});

  @override
  State<NfcReaderDialog> createState() => _NfcReaderDialogState();
}

class _NfcReaderDialogState extends State<NfcReaderDialog> with SingleTickerProviderStateMixin {
  late AnimationController _animationController;
  bool isReading = true;

  @override
  void initState() {
    super.initState();
    _animationController = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 2),
    )..repeat();
    
    _startNfcSession();
  }

  @override
  void dispose() {
    _animationController.dispose();
    NfcManager.instance.stopSession();
    super.dispose();
  }

  Future<void> _startNfcSession() async {
    try {
      await NfcManager.instance.startSession(
        onDiscovered: (NfcTag tag) async {
          // Extract NFC tag identifier
          String? nfcId;
          
          // Try to get identifier from different tag types
          if (tag.data.containsKey('nfca')) {
            final nfcA = tag.data['nfca'];
            if (nfcA['identifier'] != null) {
              nfcId = _bytesToHex(nfcA['identifier']);
            }
          } else if (tag.data.containsKey('nfcb')) {
            final nfcB = tag.data['nfcb'];
            if (nfcB['identifier'] != null) {
              nfcId = _bytesToHex(nfcB['identifier']);
            }
          } else if (tag.data.containsKey('nfcf')) {
            final nfcF = tag.data['nfcf'];
            if (nfcF['identifier'] != null) {
              nfcId = _bytesToHex(nfcF['identifier']);
            }
          } else if (tag.data.containsKey('nfcv')) {
            final nfcV = tag.data['nfcv'];
            if (nfcV['identifier'] != null) {
              nfcId = _bytesToHex(nfcV['identifier']);
            }
          }

          if (nfcId != null && nfcId.isNotEmpty) {
            await NfcManager.instance.stopSession();
            if (mounted) {
              Get.back(result: nfcId);
            }
          }
        },
      );
    } catch (e) {
      if (mounted) {
        Get.back();
        SnackbarUtils.show('오류', 'NFC 읽기 중 오류가 발생했습니다: $e');
      }
    }
  }

  String _bytesToHex(List<int> bytes) {
    return bytes.map((byte) => byte.toRadixString(16).padLeft(2, '0')).join(':').toUpperCase();
  }

  @override
  Widget build(BuildContext context) {
    return Dialog(
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
      child: Padding(
        padding: const EdgeInsets.all(32.0),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            // Animated NFC Icon
            AnimatedBuilder(
              animation: _animationController,
              builder: (context, child) {
                return Transform.scale(
                  scale: 1.0 + (_animationController.value * 0.2),
                  child: Opacity(
                    opacity: 1.0 - (_animationController.value * 0.3),
                    child: Icon(
                      Icons.nfc,
                      size: 80,
                      color: AppColors.primary,
                    ),
                  ),
                );
              },
            ),
            
            const SizedBox(height: 24),
            
            Text(
              'NFC 태그 인식 중',
              style: AppTextStyles.title,
              textAlign: TextAlign.center,
            ),
            
            const SizedBox(height: 12),
            
            Text(
              'NFC 태그를 기기 뒷면에\n가까이 대주세요',
              style: AppTextStyles.body.copyWith(color: AppColors.textSub),
              textAlign: TextAlign.center,
            ),
            
            const SizedBox(height: 32),
            
            // Cancel Button
            TextButton(
              onPressed: () {
                NfcManager.instance.stopSession();
                Get.back();
              },
              child: Text(
                '취소',
                style: AppTextStyles.button.copyWith(color: AppColors.textSub),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
