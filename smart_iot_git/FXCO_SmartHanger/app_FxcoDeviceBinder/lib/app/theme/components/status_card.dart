import 'package:flutter/material.dart';
import 'package:get/get.dart';
import '../app_colors.dart';
import '../app_text_styles.dart';
import 'app_button.dart';

class StatusCard extends StatelessWidget {
  final String title;
  final String? statusValue;
  final String? subStatusValue;
  final String actionText;
  final VoidCallback onAction;
  final bool isRecognized;
  final bool isScanning;

  const StatusCard({
    super.key,
    required this.title,
    this.statusValue,
    this.subStatusValue,
    required this.actionText,
    required this.onAction,
    this.isRecognized = false,
    this.isScanning = false,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(20),
        boxShadow: [
          BoxShadow(
            color: Colors.black.withOpacity(0.05),
            offset: const Offset(0, 4),
            blurRadius: 20,
          ),
        ],
      ),
      padding: const EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(title, style: AppTextStyles.title),
              if (isRecognized)
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
                  decoration: BoxDecoration(
                    color: AppColors.success.withOpacity(0.1),
                    borderRadius: BorderRadius.circular(20),
                  ),
                  child: Text(
                    "인식 완료",
                    style: AppTextStyles.bodySmall.copyWith(
                      color: AppColors.success,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ),
            ],
          ),
          const SizedBox(height: 16),
          if (statusValue != null) ...[
            Container(
              width: double.infinity,
              padding: const EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: AppColors.background,
                borderRadius: BorderRadius.circular(12),
              ),
              child: Column(
                children: [
                  Text(
                    statusValue!,
                    style: AppTextStyles.body.copyWith(fontFamily: 'monospace'),
                    textAlign: TextAlign.center,
                  ),
                  if (subStatusValue != null) ...[
                    const SizedBox(height: 8),
                    const Divider(height: 1),
                    const SizedBox(height: 8),
                    Text(
                      subStatusValue!,
                      style: AppTextStyles.bodySmall.copyWith(
                        fontFamily: 'monospace',
                        color: AppColors.textSub,
                      ),
                      textAlign: TextAlign.center,
                    ),
                  ],
                ],
              ),
            ),
            const SizedBox(height: 16),
          ],
          AppButton(
            text: isScanning 
                ? "인식 중..." 
                : (isRecognized ? "다시 인식하기" : actionText),
            onPressed: isScanning ? null : onAction,
            isSecondary: isRecognized,
            isLoading: isScanning,
          ),
        ],
      ),
    );
  }
}
