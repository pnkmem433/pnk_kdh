import 'dart:async';
import 'dart:convert';
import 'package:flutter_reactive_ble/flutter_reactive_ble.dart';
import 'package:permission_handler/permission_handler.dart';

class BLESettings {
  late Uuid uuid_;
  late Uuid rxChar_;
  late Uuid txChar_;

  BLESettings({required String uuid, required String rxChar, required String txChar}) {
    uuid_ = Uuid.parse(uuid);
    rxChar_ = Uuid.parse(rxChar);
    txChar_ = Uuid.parse(txChar);
  }
}


class CustomBLE {
  String _name;
  final FlutterReactiveBle _flutterReactiveBle = FlutterReactiveBle();
  List<DiscoveredDevice> _foundBleUARTDevices = [];
  late StreamSubscription<DiscoveredDevice> _scanStream;
  late Stream<ConnectionStateUpdate> _currentConnectionStream;
  late StreamSubscription<ConnectionStateUpdate> _connection;
  late QualifiedCharacteristic _txCharacteristic;
  late QualifiedCharacteristic _rxCharacteristic;
  late Stream<List<int>> _receivedDataStream;
  bool _scanning = false;
  bool _connected = false;
  final BLESettings _bleSetting;
  void Function(String value) _fetchData;

  CustomBLE({required String name, required BLESettings bleSetting, required void Function(String) fetchData})
      : _fetchData = fetchData,
        _bleSetting = bleSetting,
        _name = name;

  bool isTryToConnect() => _scanning || _connected;

  bool isConnected() => _connected;

  Future<bool> connect({Duration timeOut = const Duration(seconds: 5)}) async {
    // Request necessary permissions
    await Permission.locationWhenInUse.request();
    await Permission.bluetooth.request();
    await Permission.bluetoothScan.request();
    await Permission.bluetoothConnect.request();

    final completer = Completer<bool>();

    _foundBleUARTDevices = [];
    _scanning = true;

    // Start scanning for devices
    _scanStream = _flutterReactiveBle.scanForDevices(withServices: [_bleSetting.uuid_]).listen((device) async {
      if (_foundBleUARTDevices.every((element) => element.id != device.id)) {
        _foundBleUARTDevices.add(device);
        await Future.delayed(Duration(milliseconds: 500));

        // Stop scanning once a device is found
        await _scanStream.cancel();
        _scanning = false;

        // Start connecting to the device
        _currentConnectionStream = _flutterReactiveBle.connectToAdvertisingDevice(
          id: _foundBleUARTDevices[0].id,
          prescanDuration: const Duration(milliseconds: 500),
          withServices: [_bleSetting.uuid_, _bleSetting.rxChar_, _bleSetting.txChar_],
        );

        _connection = _currentConnectionStream.listen((event) {
          switch (event.connectionState) {
            case DeviceConnectionState.connecting:
              break;
            case DeviceConnectionState.connected:
              _connected = true;
              _txCharacteristic = QualifiedCharacteristic(
                serviceId: _bleSetting.uuid_,
                characteristicId: _bleSetting.txChar_,
                deviceId: event.deviceId,
              );
              _rxCharacteristic = QualifiedCharacteristic(
                serviceId: _bleSetting.uuid_,
                characteristicId: _bleSetting.rxChar_,
                deviceId: event.deviceId,
              );
              _receivedDataStream = _flutterReactiveBle.subscribeToCharacteristic(_txCharacteristic);
              _receivedDataStream.listen((data) => _fetchData(String.fromCharCodes(data)), onError: (dynamic error) {});
              if (!completer.isCompleted) {
                completer.complete(true);
              }
              break;
            case DeviceConnectionState.disconnecting:
              _connected = false;
              break;
            case DeviceConnectionState.disconnected:
              _connected = false;
              if (!completer.isCompleted) {
                completer.complete(false);
              }
              break;
          }
        });
      }
    }, onError: (Object error) {
      if (!completer.isCompleted) {
        completer.complete(false);
      }
    });

    // Complete the connection process within the given timeout
    Future.delayed(timeOut, () {
      if (!completer.isCompleted) {
        completer.complete(false);
      }
    });

    return completer.future;
  }

  Future<bool> disconnect() async {
    if (_connected) {
      await _connection.cancel();
      _connected = false;
      return true;
    }
    return false;
  }

  Future<bool> cancel() async {
    if (_scanning) {
      await _scanStream.cancel();
      _scanning = false;
    }
    return true;
  }

  Future<bool> sendString(String text) async {
    if (_connected) {
      List<int> data = utf8.encode(text);
      try {
        await _flutterReactiveBle.writeCharacteristicWithoutResponse(_rxCharacteristic, value: data);
        return true;
      } catch (e) {
        print('Error sending data: $e');
        return false;
      }
    }
    return false;
  }
}
