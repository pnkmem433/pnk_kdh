import 'package:flutter/material.dart';
import 'package:get/get.dart';
import '../../theme/app_colors.dart';

class SnackbarUtils {
  static void show(String title, String message) {
    // Determine color based on title or message content
    final bool isSuccess = title.toLowerCase().contains('success') || 
                           message.contains('성공');
    
    Get.snackbar(
      title,
      message,
      backgroundColor: isSuccess ? AppColors.success : AppColors.error,
      colorText: AppColors.white,
      snackPosition: SnackPosition.BOTTOM,
      margin: const EdgeInsets.all(16),
      borderRadius: 12,
      duration: const Duration(seconds: 3),
    );
  }
}
