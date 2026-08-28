import 'package:fxco_stylingbooth/shared/models/dto/rfid_scan_dto.dart';
import 'package:http/http.dart' as http;

import '../../core/config/database_table.dart';

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
      print("RfidScanProvider > readRfidScannedBySeq 오류 발생: $e");
      return null;
    }
  }
}