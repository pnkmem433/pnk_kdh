import 'dart:convert';

import 'package:fxco_stylingbooth/shared/models/dto/rfid_scan_dto.dart';
import 'package:http/http.dart' as http;

import '../../core/config/database_table.dart';
import '../models/dto/rfid_all_data_dto.dart';

class RfidScanProvider {
  Future<RfidScan?> readRfidScannedBySeq({
    required int sessionSeq,
  }) async {
    var url = Uri.parse(DatabaseTable.rfidScannedBySessionSeq+sessionSeq.toString());

    try {
      var response = await http.get(url);
      if (response.statusCode == 200) {
        RfidScan result = rfidScanFromJson(response.body);
        return result;
      } else {
        throw Exception('Failed to readRfidScannedBySeq');
      }
    } catch (e) {
      print("오류 발생: $e");
      return null;
    }
  }

  Future<RfidAllData?> readAllRfidScannedDataBySeq({
    required int sessionSeq,
  }) async {
    var url = Uri.parse(DatabaseTable.readAllRfidScannedDataBySeq+sessionSeq.toString());

    try {
      var response = await http.get(url);
      if (response.statusCode == 200) {
        RfidAllData result = rfidAllDataFromJson(response.body);
        return result;
      } else {
        throw Exception('Failed to readAllRfidScannedDataBySeq');
      }
    } catch (e) {
//      print("오류 발생: $e");
      return null;
    }
  }


  Future<bool> createRfidScan({
    required String qrCode,
    required int sessionSeq,
  }) async {
    Future<Map<String, dynamic>> _returnMap() async {
      return {
        "session_seq": sessionSeq,
        "clothes_product_id": qrCode,
        "scan_source_type": 1,
      };
    }

    var url = Uri.parse(DatabaseTable.createRfidScan);
    final headers = {'Content-Type': 'application/json'};
    final body = json.encode(await _returnMap());

    try {
      final response = await http.post(url, headers: headers, body: body);
      print('Response status code: ${response.statusCode}');

      if (response.statusCode == 201) {
        return true;
      } else {
        throw Exception('Failed to save data');
      }
    } catch (e) {
      print("createRfidScan 오류 발생: $e");
      return false;
    }
  }
}