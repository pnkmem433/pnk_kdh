import 'package:flutter/material.dart';
import 'package:get/get.dart';
import '../../theme/app_colors.dart';
import '../../theme/app_text_styles.dart';
import '../../theme/components/app_button.dart';
import 'smart_hanger_controller.dart';

class SmartHangerPage extends StatelessWidget {
  const SmartHangerPage({super.key});

  @override
  Widget build(BuildContext context) {
    // Inject controller specific to this page
    final controller = Get.put(SmartHangerController());

    return Padding(
      padding: const EdgeInsets.all(20.0),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          // Header Status
          Obx(() {
            final isConnected = controller.mqttConnected.value;
            final isSubscribed = controller.mqttSubscribed.value;
            final isConnecting = controller.isConnecting.value;
            final hasScanned = controller.scannedQr.value.isNotEmpty;
            final isMatched = controller.isClothesMatched.value;
            final topic = controller.mqttTopicForDisplay;

            return Container(
              padding: const EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: Colors.white,
                borderRadius: BorderRadius.circular(16),
              ),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  if (controller.scannedQr.isNotEmpty)
                    Align(
                      alignment: Alignment.centerRight,
                      child: Padding(
                        padding: const EdgeInsets.only(bottom: 4.0),
                        child: TextButton(
                        style: TextButton.styleFrom(
                          minimumSize: Size.zero,
                          padding: EdgeInsets.zero,
                          tapTargetSize: MaterialTapTargetSize.shrinkWrap,
                        ),
                        onPressed: hasScanned ? controller.reset : null,
                        child: Text(
                          "Reset",
                          style: AppTextStyles.bodySmall.copyWith(
                            color: AppColors.textSub,
                          ),
                        ),
                      ),
                      ),
                    ),
                  // 스마트 헹거 인식 상태
                  if (!isConnecting && hasScanned)
                    Padding(
                      padding: const EdgeInsets.only(bottom: 12.0),
                      child: _buildStatusChip(
                        label: (isConnected && isSubscribed) ? "스마트 헹거 인식 성공" : "스마트 헹거 인식 실패",
                        ok: isConnected && isSubscribed,
                      ),
                    ),
                  // 옷 연결 상태
                  if (isMatched != null)
                    Padding(
                      padding: const EdgeInsets.only(bottom: 12.0),
                      child: _buildStatusChip(
                        label: isMatched ? "옷 연결 성공" : "옷 연결 실패",
                        ok: isMatched,
                      ),
                    ),
                  // 어떤 토픽에 연결하는지 보여준다
                  // if (topic.isNotEmpty)
                  //   Padding(
                  //     padding: const EdgeInsets.only(top: 6),
                  //     child: Text(
                  //       "Topic: $topic",
                  //       style: AppTextStyles.bodySmall,
                  //       textAlign: TextAlign.center,
                  //     ),
                  //   ),
                  // if (!isConnected)
                  //   Padding(
                  //     padding: const EdgeInsets.only(top: 4),
                  //     child: Text(
                  //       "state=${controller.mqttConnectionStateLabel.value} / code=${controller.mqttConnectionReturnCodeLabel.value}",
                  //       style: AppTextStyles.bodySmall,
                  //       textAlign: TextAlign.center,
                  //     ),
                  //   ),
                  // const SizedBox(height: 10),
                  Text(
                    controller.statusMessage.value,
                    style: AppTextStyles.title,
                    textAlign: TextAlign.center,
                  ),
                  if (controller.scannedQr.isNotEmpty)
                    Padding(
                      padding: const EdgeInsets.only(top: 8.0),
                      child: Text(
                        /*"QR: "+*/"${controller.scannedQr.value}",
                        style: AppTextStyles.bodySmall,
                        textAlign: TextAlign.center,
                      ),
                    ),
                ],
              ),
            );
          }),
          const SizedBox(height: 20),
          
          // Main Content Area
          Expanded(
            child: Obx(() {
               switch (controller.currentStep.value) {
                 case HangerStep.scan:
                   return Center(
                     child: Column(
                       mainAxisSize: MainAxisSize.min,
                       children: [
                         AppButton(
                           text: "QR코드 스캔하기",
                           onPressed: controller.startScan,
                         ),
                         // MQTT 연결 확인을 위한 Test 버튼
                        //  const SizedBox(height: 12),
                        //  AppButton(
                        //    text: "MQTT 연결하기",
                        //    onPressed: controller.subscribeFixedTopic,
                        //    isSecondary: true,
                        //  ),
                       ],
                     ),
                   );
                   
                 case HangerStep.select:
                   return GridView.builder(
                     gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                       crossAxisCount: 2,
                       crossAxisSpacing: 10,
                       mainAxisSpacing: 10,
                       childAspectRatio: 0.8,
                     ),
                     itemCount: controller.clothesList.length,
                     itemBuilder: (context, index) {
                       return Obx(() {
                         final isSelected = controller.selectedClothesIndex.value == index;
                         return GestureDetector(
                           onTap: () => controller.selectClothes(index),
                           child: Container(
                             decoration: BoxDecoration(
                               color: isSelected ? AppColors.primary.withOpacity(0.1) : Colors.white,
                               borderRadius: BorderRadius.circular(12),
                               border: Border.all(
                                 color: isSelected ? AppColors.primary : Colors.transparent,
                                 width: 2,
                               ),
                             ),
                             child: Column(
                               mainAxisAlignment: MainAxisAlignment.center,
                               children: [
                                 Icon(
                                   Icons.checkroom,
                                   size: 48,
                                   color: isSelected ? AppColors.primary : AppColors.textSub,
                                 ),
                                 const SizedBox(height: 12),
                                 Text(
                                   controller.clothesList[index],
                                   style: isSelected ? AppTextStyles.body.copyWith(fontWeight: FontWeight.bold) : AppTextStyles.body,
                                 ),
                               ],
                             ),
                           ),
                         );
                       });
                     },
                   );
                   
                 case HangerStep.listen:
                   return Column(
                     children: [
                        // Expanded(
                        //   child: Container(
                        //     width: double.infinity,
                        //     padding: const EdgeInsets.all(16),
                        //     margin: const EdgeInsets.only(bottom: 20),
                        //     decoration: BoxDecoration(
                        //       color: Colors.black87,
                        //       borderRadius: BorderRadius.circular(12),
                        //     ),
                        //     child: Obx(() => ListView.builder(
                        //       itemCount: controller.eventLog.length,
                        //       itemBuilder: (context, index) => Padding(
                        //         padding: const EdgeInsets.symmetric(vertical: 4),
                        //         child: Text(
                        //           controller.eventLog[index],
                        //           style: const TextStyle(color: Colors.greenAccent, fontFamily: 'monospace'),
                        //         ),
                        //       ),
                        //     )),
                        //   ),
                        // ),
                        // Mock Trigger Button in Listen Mode for Debugging
                        // OutlinedButton(
                        //   onPressed: controller.debugSimulateMqtt,
                        //   child: const Text("Simulate MQTT Event (Debug)"),
                        // ),
                     ],
                   );
               }
            }),
          ),
          
          // Bottom Action
          const SizedBox(height: 20),
          Obx(() {
            if (controller.currentStep.value == HangerStep.select) {
              return AppButton(
                text: "저장하기",
                onPressed: controller.selectedClothesIndex.value != -1 
                    ? controller.saveAssociation 
                    : null,
              );
            }
            return const SizedBox.shrink();
          }),
        ],
      ),
    );
  }

  Widget _buildStatusChip({required String label, required bool? ok}) {
    Color borderColor;
    Color backgroundColor;

    if (ok == true) {
      borderColor = AppColors.success;
      backgroundColor = AppColors.success.withOpacity(0.12);
    } else if (ok == false) {
      borderColor = Colors.redAccent;
      backgroundColor = Colors.redAccent.withOpacity(0.08);
    } else {
      // In Progress / Initial (Grey)
      borderColor = AppColors.textSub;
      backgroundColor = AppColors.textSub.withOpacity(0.1);
    }

    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 8),
      decoration: BoxDecoration(
        color: backgroundColor,
        borderRadius: BorderRadius.circular(10),
        border: Border.all(color: borderColor, width: 1.2),
      ),
      child: Text(
        label,
        textAlign: TextAlign.center,
        style: AppTextStyles.bodySmall.copyWith(
          color: borderColor,
          fontWeight: FontWeight.w700,
        ),
      ),
    );
  }
}
