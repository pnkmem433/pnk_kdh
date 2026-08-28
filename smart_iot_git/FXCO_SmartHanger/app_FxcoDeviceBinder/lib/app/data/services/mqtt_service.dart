import 'dart:async';
import 'dart:convert';
import 'package:flutter/foundation.dart';
import 'package:get/get.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

class MqttService extends GetxService {
  MqttServerClient? _client;
  final _eventController = StreamController<Map<String, dynamic>>.broadcast();
  Stream<Map<String, dynamic>> get eventStream => _eventController.stream;

  // Reactive connection/subscription status for UI
  final RxBool isConnected = false.obs;
  final RxBool isSubscribed = false.obs;
  final RxString subscribedTopic = ''.obs;
  final RxString connectionStateLabel = 'disconnected'.obs;
  final RxString connectionReturnCodeLabel = 'none'.obs;

  Future<bool> connect(
    String broker,
    String username,
    String password, {
    String? clientId,
    int? port,
  }) async {
    try {
      if (_client?.connectionStatus?.state == MqttConnectionState.connected) {
        return true;
      }

      final id = clientId ?? 'flutter_client_${DateTime.now().millisecondsSinceEpoch}';
      final uri = _normalizeBroker(broker, fallbackPort: port ?? 1883);
      _client = MqttServerClient(uri.host, id);
      _client!.port = uri.port;
      _client!.logging(on: kDebugMode);
      _client!.keepAlivePeriod = 20;
      _client!.autoReconnect = true;
      _client!.setProtocolV311();
      _client!.onDisconnected = _onDisconnected;
      _client!.onConnected = _onConnected;
      _client!.onSubscribed = _onSubscribed;
      
      final connMessage = MqttConnectMessage()
          .withClientIdentifier(id)
          .authenticateAs(username, password)
          .startClean();
      
      _client!.connectionMessage = connMessage;

      print('[MQTT] Connecting to ${uri.host}:${uri.port} (from: $broker)...');
      await _client!.connect();
      _updateConnectionStatusLabels();

      if (_client!.connectionStatus!.state == MqttConnectionState.connected) {
        print('[MQTT] Connected successfully');
        isConnected.value = true;
        _client!.updates!.listen(_onMessage);
        return true;
      } else {
        print('[MQTT] Connection failed - status: ${_client!.connectionStatus!.state}');
        isConnected.value = false;
        isSubscribed.value = false;
        _client!.disconnect();
        return false;
      }
    } catch (e) {
      print('[MQTT] Exception: $e');
      isConnected.value = false;
      isSubscribed.value = false;
      _updateConnectionStatusLabels();
      _client?.disconnect();
      return false;
    }
  }

  /// Accepts either "192.168.1.67", "192.168.1.67:1883", or "mqtt://192.168.1.67:1883"
  /// and returns a normalized Uri with host/port populated.
  Uri _normalizeBroker(String broker, {required int fallbackPort}) {
    final trimmed = broker.trim();
    final withScheme = trimmed.contains('://') ? trimmed : 'mqtt://$trimmed';
    final parsed = Uri.parse(withScheme);
    final host = parsed.host.isEmpty ? parsed.path : parsed.host;
    final port = parsed.hasPort ? parsed.port : fallbackPort;
    return Uri(scheme: parsed.scheme.isEmpty ? 'mqtt' : parsed.scheme, host: host, port: port);
  }

  void subscribe(String topic) {
    if (_client?.connectionStatus?.state == MqttConnectionState.connected) {
      print('[MQTT] Subscribing to $topic');
      isSubscribed.value = false;
      _client!.subscribe(topic, MqttQos.atMostOnce);
    } else {
      print('[MQTT] Cannot subscribe - not connected');
      isSubscribed.value = false;
    }
  }

  void publishJson(String topic, Map<String, dynamic> payload) {
    if (_client?.connectionStatus?.state != MqttConnectionState.connected) {
      print('[MQTT] Cannot publish - not connected');
      return;
    }

    final builder = MqttClientPayloadBuilder();
    final message = jsonEncode(payload);
    builder.addString(message);

    print('[MQTT] Publishing to $topic: $message');
    _client!.publishMessage(topic, MqttQos.atLeastOnce, builder.payload!);
  }

  void _onMessage(List<MqttReceivedMessage<MqttMessage?>>? c) {
    final MqttPublishMessage recMess = c![0].payload as MqttPublishMessage;
    final String pt = MqttPublishPayload.bytesToStringAsString(recMess.payload.message);
    
    // print('[MQTT] Mock/Real msg received: topic=${c[0].topic}, payload=$pt');
    
    try {
      final Map<String, dynamic> payload = jsonDecode(pt);
      _eventController.add(payload);
    } catch (e) {
      print('[MQTT] Failed to parse message: $e');
    }
  }

  void _onConnected() {
    print('[MQTT] OnConnected client callback - Client connection was successful');
    isConnected.value = true;
    _updateConnectionStatusLabels();
  }

  void _onDisconnected() {
    final status = _client?.connectionStatus;
    final state = status?.state;
    final code = status?.returnCode;
    print('[MQTT] OnDisconnected client callback - state: $state, returnCode: $code');
    isConnected.value = false;
    isSubscribed.value = false;
    _updateConnectionStatusLabels();
  }

  void disconnect() {
    print('[MQTT] Manual disconnect requested');
    _client?.disconnect();
    _onDisconnected();
  }

  void _onSubscribed(String topic) {
    print('[MQTT] Subscription confirmed for topic: "$topic"');
    subscribedTopic.value = topic;
    isSubscribed.value = true;
  }

  void _updateConnectionStatusLabels() {
    final status = _client?.connectionStatus;
    final state = status?.state;
    final code = status?.returnCode;

    connectionStateLabel.value = state == null
        ? 'unknown'
        : state.toString().split('.').last;
    connectionReturnCodeLabel.value = code == null
        ? 'none'
        : code.toString().split('.').last;
  }

  /// Helper to manually simulate an event for testing
  void simulateEvent(String topic, Map<String, dynamic> payload) {
    print("[MQTT] Simulating event on $topic: $payload");
    _eventController.add(payload);
  }
  
  @override
  void onClose() {
    _client?.disconnect();
    _eventController.close();
    super.onClose();
  }
}
