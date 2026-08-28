import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'package:fxco_stylingbooth/core/theme/colors.dart';
import 'package:fxco_stylingbooth/core/theme/icons.dart';
import 'package:fxco_stylingbooth/core/theme/layout_constants.dart';
import 'package:fxco_stylingbooth/core/theme/text_style.dart';
import 'package:fxco_stylingbooth/shared/widgets/qr_code.dart';
import 'package:get/get.dart';
import '../../shared/widgets/qr/scanner_widget.dart';
import 'qr_display_controller.dart';

class QRDisplayView extends StatelessWidget {
  const QRDisplayView({super.key});
  @override
  Widget build(BuildContext context) {
    final controller = Get.put(QRDisplayController());

    return Scaffold(
      body: Obx(() {
        if (!controller.isRfidDataLoaded.value) {
          return const Center(child: CircularProgressIndicator()); // 또는 FadeInDelay()
        }

        // 데이터 로드 완료 후 판단
        return controller.rfidAllData.value.isNull
            ? buildNotRfidScannedWidget(controller, context)
            : buildScannedWidget(controller);
      }),
    );

    return Scaffold(
      body: Obx(() => controller.rfidAllData.value.isNull
          ? buildNotRfidScannedWidget(controller, context)
          : buildScannedWidget(controller)
      ),
    );
  }

  Row buildScannedWidget(QRDisplayController controller) {
    return Row(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        // 왼쪽 영역 (QR 코드)
        Expanded(
          flex: 5,
          child: Container(
            width: Get.width,
            alignment: Alignment.center,
            padding: const EdgeInsets.symmetric(horizontal: AppLayout.spaceEdge),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.center,
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Text('완벽한 당신이 완성되었어요.', style: AppTextStyles.largeTitle, textAlign: TextAlign.center,),
                Text('앱으로 QR을 스캔하고,', style: AppTextStyles.largeTitle, textAlign: TextAlign.center,),
                Text.rich(
                  TextSpan(
                    children: [
                      TextSpan(
                        text: 'INST',
                        style: AppTextStyles.largeTitle.copyWith(fontWeight: FontWeight.bold),
                      ),
                      TextSpan(
                        text: '의 특별한 순간을 만나보세요.',
                        style: AppTextStyles.largeTitle,
                      ),
                    ],
                  ),
                ),
                AppLayout.vSpaceXl,
                AppLayout.vSpaceXl,
                QrCode(content: "choibokoatc://qr?store_id=FXCO&product_id=${controller.newQRCode[controller.rfidAllData.value!.clothesProductId]}"),
              ],
            ),
          ),
        ),

        Container(
          height: Get.height*0.5,
          width: 1,
          color: AppColors.adaptiveGrey300,
        ),

        // 오른쪽 영역 (버튼)
        Expanded(
          flex: 2,
          child: Obx(() {
            final index = controller.currentIndex.value;

            return Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                // 이미지 전환
                AnimatedSwitcher(
                  duration: const Duration(milliseconds: 800),
                  transitionBuilder: (child, animation) => FadeTransition(
                    opacity: animation,
                    child: child,
                  ),
                  child: Image.asset(
                    controller.images[index],
                    key: ValueKey<int>(index),
                    width: 240,
                  ),
                ),
                AppLayout.vSpaceXl,

                // 설명 문구 전환
                AnimatedSwitcher(
                  duration: const Duration(milliseconds: 800),
                  transitionBuilder: (child, animation) => FadeTransition(
                    opacity: animation,
                    child: child,
                  ),
                  child: Text(
                    controller.captions[index],
                    key: ValueKey<int>(index),
                    style: AppTextStyles.smallTitle,
                    textAlign: TextAlign.center,
                  ),
                ),
              ],
            );
          })
        ),
      ],
    );
  }

  Center buildNotRfidScannedWidget(QRDisplayController controller, BuildContext context) {
    return Center(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.center,
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          AppLayout.vSpaceXl,
          AppLayout.vSpaceXl,
          AppLayout.vSpaceXl,
          AppLayout.vSpaceXl,
          AppLayout.vSpaceXl,
          AppLayout.vSpaceXl,
          Text('QR을 스캔해 주시면', style: AppTextStyles.largeTitle.copyWith(fontWeight: FontWeight.w600),),
          AppLayout.vSpaceMd,
          const Text('진짜 당신다운 INST 스타일이 완성됩니다.', style: AppTextStyles.largeTitle, textAlign: TextAlign.center,),
          AppLayout.vSpaceXl,
          AppLayout.vSpaceXl,
          GestureDetector(
            onTap: (){
              controller.clickNextStepEnd();
              _showScannerDialog(context, controller);
            },
            child: Container(
                padding: const EdgeInsets.symmetric(vertical: AppLayout.spaceXl, horizontal: AppLayout.spaceXl),
                decoration: const BoxDecoration(
                  color: AppColors.adaptiveGrey700,
                  borderRadius: AppLayout.radiusMd,
                ),
                child: Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    SvgPicture.asset(AppIcons.icScannerLine, color: AppColors.adaptiveGrey50, width: 48.0, height: 48.0,),
                    AppLayout.hSpaceLg,
                    Text('옷의 QR 코드 스캔하기', style: AppTextStyles.mediumTitle.copyWith(color: AppColors.white),),
                  ],
                )
            ),
          ),
          AppLayout.vSpaceXl,
          AppLayout.vSpaceXl,
          AppLayout.vSpaceXl,
          AppLayout.vSpaceXl,
          Container(
            padding: EdgeInsets.only(right: 60),
            alignment: Alignment.centerRight,
            child: Text('옷이 없다면 옷을 소지한 후 다시 이용해주세요', style: AppTextStyles.smallTitle.copyWith(color: AppColors.adaptiveGrey500), textAlign: TextAlign.center,),
          )
        ],
      ),
    );
  }

  void _showScannerDialog(BuildContext context, QRDisplayController controller) {
    showDialog(
      context: context,
      barrierDismissible: true,
      builder: (context) {
        return StatefulBuilder(
          builder: (context, setState) {
            return Dialog(
              backgroundColor: AppColors.white,
              surfaceTintColor: AppColors.white,
              shadowColor: AppColors.adaptiveGrey500,
              insetPadding: AppLayout.paddingSm,
              shape: const RoundedRectangleBorder(
                borderRadius: AppLayout.radiusLg,
              ),
              child: Padding(
                padding: AppLayout.paddingXl,
                child:
                Obx(() => !controller.showNextStep.value
                    ? Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Text(
                      '옷의 QR을 스캔해주세요.',
                      style: AppTextStyles.mediumTitle.copyWith(fontWeight: FontWeight.w600),
                    ),
                    AppLayout.vSpaceMd,
                    const Text(
                      '‘다음’을 클릭하면 카메라가 켜져요.',
                      style: AppTextStyles.mediumTitle,
                    ),
                    AppLayout.vSpaceXl,

                    GestureDetector(
                      onTap: (){
                        controller.clickNextStep();
                      },
                      child: Container(
                          padding: const EdgeInsets.symmetric(vertical: AppLayout.spaceLg, horizontal: AppLayout.spaceXl),
                          decoration: const BoxDecoration(
                            color: AppColors.adaptiveGrey100,
                            borderRadius: AppLayout.radiusMd,
                          ),
                          child: Text('다음', style: AppTextStyles.mediumTitle.copyWith(color: AppColors.adaptiveGrey500),)
                      ),
                    ),
                  ],
                ) :
                Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    RotatedBox(quarterTurns: 3, child: ScannerWidget(
                      isCaptured: controller.createRfidScan,
                      size: const Size(300, 300),
                    ),),

                    AppLayout.vSpaceLg,

                    GestureDetector(
                      onTap: () {
                        Navigator.of(context).pop();
                        /// to-do : QR코드 스캐너 추가
//                        controller.createRfidScan(qrCode: "IT00013");
//                          controller.refreshQRData();
                      },
                      child: Container(
                          padding: const EdgeInsets.symmetric(vertical: AppLayout.spaceLg, horizontal: AppLayout.spaceXl),
                          decoration: const BoxDecoration(
                            color: AppColors.adaptiveGrey100,
                            borderRadius: AppLayout.radiusMd,
                          ),
                          child: Text('닫기', style: AppTextStyles.mediumTitle.copyWith(color: AppColors.adaptiveGrey500),)
                      ),
                    ),
                  ],
                )),
              ),
            );
          },
        );
      },
    );
  }
}
