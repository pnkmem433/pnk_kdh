import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:webview_flutter/webview_flutter.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  SystemChrome.setEnabledSystemUIMode(SystemUiMode.immersive);
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return const MaterialApp(
      debugShowCheckedModeBanner: false,
      home: WebShell(),
    );
  }
}

class WebShell extends StatefulWidget {
  const WebShell({super.key});

  @override
  State<WebShell> createState() => _WebShellState();
}

class _WebShellState extends State<WebShell> {
  late final WebViewController _controller;

  @override
  void initState() {
    super.initState();

    _controller = WebViewController()
      ..setJavaScriptMode(JavaScriptMode.unrestricted)
      ..setNavigationDelegate(
        NavigationDelegate(
          onPageStarted: (url) =>
              debugPrint('[WebView] start: $url'),
          onPageFinished: (url) =>
              debugPrint('[WebView] finish: $url'),
          onNavigationRequest: (req) {
            debugPrint('[WebView] nav: ${req.url}');
            return NavigationDecision.navigate;
          },
          onWebResourceError: (e) {
            debugPrint(
              '[WebView][ERROR] '
                  'code=${e.errorCode} '
                  'type=${e.errorType} '
                  'desc=${e.description} '
                  'mainFrame=${e.isForMainFrame}',
            );
          },
        ),
      )
      ..loadRequest(Uri.parse('http://192.168.1.67/hmi/'));
  }

  @override
  Widget build(BuildContext context) {
    // Scaffold도 최소 구성: AppBar 없음, 안전영역만 적용
    return WillPopScope(
        onWillPop: ()  {
          return Future(() => false); //뒤로가기 막음
        },
        child: SafeArea(
          top: false,
          bottom: false,
          child: WebViewWidget(controller: _controller),
        )
    );
  }
}
