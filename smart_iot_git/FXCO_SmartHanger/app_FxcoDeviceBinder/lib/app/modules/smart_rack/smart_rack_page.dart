import 'package:flutter/material.dart';
import 'package:get/get.dart';
import '../../theme/app_colors.dart';
import '../../theme/app_text_styles.dart';
import '../../theme/components/app_button.dart';
import '../../theme/components/status_card.dart';
import 'smart_rack_controller.dart';

class SmartRackPage extends StatelessWidget {
  const SmartRackPage({super.key});

  @override
  Widget build(BuildContext context) {
    // Inject logic
    final controller = Get.put(SmartRackController());

    return Padding(
      padding: const EdgeInsets.all(20.0),
      child: Column(
        children: [
          Expanded(
            child: SingleChildScrollView(
              child: Column(
                children: [
                  Obx(() => StatusCard(
                    title: "스마트 헹거랙",
                    statusValue: controller.smartRackId.value,
                    subStatusValue: controller.canonicalSmartRackId,
                    actionText: "NFC 인식하기",
                    isRecognized: controller.isSmartRackRecognized,
                    isScanning: controller.isSmartRackScanning.value,
                    onAction: controller.scanSmartRack,
                  )),
                  const SizedBox(height: 16),
                  Obx(() {
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
                          Text("헹거랙", style: AppTextStyles.title),
                          const SizedBox(height: 16),
                          Wrap(
                            spacing: 8,
                            runSpacing: 8,
                            children: List.generate(14, (i) {
                              final number = i + 1;
                              final isSelected =
                                  controller.selectedPosition.value == number;
                              return SizedBox(
                                width: 56,
                                height: 56,
                                child: OutlinedButton(
                                  onPressed: () => controller.selectPosition(number),
                                  style: OutlinedButton.styleFrom(
                                    padding: EdgeInsets.zero,
                                    backgroundColor: Colors.white,
                                    side: BorderSide(
                                      color: isSelected
                                          ? AppColors.primary
                                          : Colors.transparent,
                                      width: 2,
                                    ),
                                    shape: RoundedRectangleBorder(
                                      borderRadius: BorderRadius.circular(12),
                                    ),
                                  ),
                                  child: Text(
                                    "$number",
                                    style: AppTextStyles.body.copyWith(
                                      color: isSelected
                                          ? AppColors.primary
                                          : AppColors.textMain,
                                      fontWeight: FontWeight.w600,
                                    ),
                                  ),
                                ),
                              );
                            }),
                          ),
                        ],
                      ),
                    );
                  }),
                ],
              ),
            ),
          ),
          const SizedBox(height: 20),
          Obx(() => AppButton(
            text: "저장하기",
            onPressed: controller.canSave ? controller.savePair : null,
            isLoading: controller.isSaving.value,
          )),
        ],
      ),
    );
  }
}
