import 'package:flutter/material.dart';
import '../app_colors.dart';
import '../app_text_styles.dart';

class AppButton extends StatelessWidget {
  final String text;
  final VoidCallback? onPressed;
  final bool isSecondary;
  final bool isLoading;
  final bool isFullWidth;

  const AppButton({
    super.key,
    required this.text,
    required this.onPressed,
    this.isSecondary = false,
    this.isLoading = false,
    this.isFullWidth = true,
  });

  @override
  Widget build(BuildContext context) {
    final backgroundColor = isSecondary ? AppColors.secondary : AppColors.primary;
    final foregroundColor = isSecondary ? AppColors.primary : Colors.white;
    
    return SizedBox(
      width: isFullWidth ? double.infinity : null,
      height: 52,
      child: ElevatedButton(
        style: ElevatedButton.styleFrom(
          backgroundColor: backgroundColor,
          foregroundColor: foregroundColor,
          elevation: 0,
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(12),
          ),
          padding: const EdgeInsets.symmetric(horizontal: 24),
        ),
        onPressed: isLoading ? null : onPressed,
        child: isLoading
            ? SizedBox(
                height: 24,
                width: 24,
                child: CircularProgressIndicator(
                  strokeWidth: 2,
                  valueColor: AlwaysStoppedAnimation<Color>(foregroundColor),
                ),
              )
            : Text(
                text,
                style: AppTextStyles.button.copyWith(
                  color: foregroundColor,
                ),
              ),
      ),
    );
  }
}
