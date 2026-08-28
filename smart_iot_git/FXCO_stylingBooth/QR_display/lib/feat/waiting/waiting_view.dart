import 'package:flutter/material.dart';
import 'package:fxco_stylingbooth/core/theme/colors.dart';
import 'package:fxco_stylingbooth/core/theme/layout_constants.dart';
import 'package:fxco_stylingbooth/core/theme/text_style.dart';
import 'package:get/get.dart';
import 'package:video_player/video_player.dart';
import '../../app_state_controller.dart';
import 'waiting_controller.dart';

class WaitingView extends StatelessWidget {
  const WaitingView({super.key});

  @override
  Widget build(BuildContext context) {
    final controller = Get.find<WaitingController>();

    return Scaffold(
      body: Center(
          child: Obx(() {
            if (!controller.isVideoReady.value) {
              // 초기화 중일 때 로딩 표시
              return const CircularProgressIndicator();
            }

            // 비디오 초기화 실패 시 처리
            if (controller.hasVideoError.value) {
              return _buildContentWithoutVideo(controller);
            }

            final vp = controller.videoPlayerController;

            return Stack(
              children: [
                // 영상 - 초기화 완료 여부 확인
                if (vp.value.isInitialized)
                  Positioned.fill(
                    child: FittedBox(
                      fit: BoxFit.cover,
                      child: SizedBox(
                        width: vp.value.size.width,
                        height: vp.value.size.height,
                        child: VideoPlayer(vp),
                      ),
                    ),
                  ),

                // 반투명 화이트 오버레이 - 화면 전체를 덮음
                Positioned.fill(
                  child: IgnorePointer(
                    child: Container(color: Colors.white.withOpacity(0.3)),
                  ),
                ),

                // 중앙 콘텐츠
                Center(child: _buildCenterContent(controller)),
              ],
            );
          })
      ),
    );
  }

  Widget _buildContentWithoutVideo(WaitingController controller) {
    return Container(
      color: Colors.white,
      child: _buildCenterContent(controller),
    );
  }

  Widget _buildCenterContent(WaitingController controller) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.center,
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        AppLayout.vSpaceXl,
        AppLayout.vSpaceXl,
        AppLayout.vSpaceXl,
        Opacity(
          opacity: controller.isScanned.value ? 1 : 0.2,
          child: Padding(
            padding: const EdgeInsets.only(left: 24.0),
            child: Image.asset(
              'assets/images/INST_logo_b.png',
              width: 640,
            ),
          ),
        ),
        AppLayout.vSpaceXl,
        Obx(() {
          if (controller.isScanned.value) {
            return Text(
              "입고계신 옷을 확인하고 있어요 ${controller.countdown.value}",
              style: AppTextStyles.smallTitle,
            );
          }

          if (controller.showAlertMessage.value) {
            return Text(
              "퇴장 후 재입장해주세요.",
              style: AppTextStyles.smallTitle.copyWith(
                color: AppColors.adaptiveRed600,
                fontWeight: FontWeight.bold,
              ),
            );
          }

          return const Text("", style: AppTextStyles.smallTitle);
        }),
      ],
    );
  }
}