import 'package:flutter/material.dart';
import 'package:get/get.dart';
import 'app/theme/app_theme.dart';
import 'app/modules/home/home_binding.dart';
import 'app/modules/home/home_page.dart';
import 'app/data/services/api_service.dart';
import 'app/data/services/mqtt_service.dart';
import 'app/data/services/scan_service.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await initServices();
  runApp(const MyApp());
}

Future<void> initServices() async {
  Get.put(ApiService());
  Get.put(MqttService());
  Get.put(ScanService());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return GetMaterialApp(
      title: 'FXCO Device Binder',
      debugShowCheckedModeBanner: false,
      theme: AppTheme.light,
      initialBinding: HomeBinding(),
      home: const HomePage(),
    );
  }
}
