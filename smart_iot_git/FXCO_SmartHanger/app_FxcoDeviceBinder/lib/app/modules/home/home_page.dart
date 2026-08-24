import 'package:flutter/material.dart';
import 'package:get/get.dart';
import '../../theme/app_colors.dart';
import '../../theme/app_text_styles.dart';
import '../smart_hanger/smart_hanger_page.dart';
import '../smart_rack/smart_rack_page.dart';
import 'home_controller.dart';

class HomePage extends GetView<HomeController> {
  const HomePage({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text("기기 페어링 관리"),
        bottom: TabBar(
          controller: controller.tabController,
          labelColor: AppColors.primary,
          unselectedLabelColor: AppColors.textSub,
          labelStyle: AppTextStyles.title.copyWith(fontSize: 16),
          indicatorColor: AppColors.primary,
          indicatorWeight: 3,
          tabs: controller.tabs.map((t) => Tab(text: t)).toList(),
        ),
      ),
      body: TabBarView(
        controller: controller.tabController,
        children: const [
          SmartHangerPage(),
          SmartRackPage(),
        ],
      ),
    );
  }
}
