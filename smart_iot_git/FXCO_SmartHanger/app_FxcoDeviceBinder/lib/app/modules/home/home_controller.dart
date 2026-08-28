import 'package:flutter/material.dart';
import 'package:get/get.dart';

class HomeController extends GetxController with GetSingleTickerProviderStateMixin {
  late TabController tabController;
  
  final List<String> tabs = ["스마트 헹거", "스마트 헹거랙"];

  @override
  void onInit() {
    super.onInit();
    tabController = TabController(length: tabs.length, vsync: this);
  }

  @override
  void onClose() {
    tabController.dispose();
    super.onClose();
  }
}
