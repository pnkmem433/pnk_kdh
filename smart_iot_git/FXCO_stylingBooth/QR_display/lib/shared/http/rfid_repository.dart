import 'package:fxco_stylingbooth/shared/http/rfid_provider.dart';

import '../models/dto/rfid_all_data_dto.dart';
import '../models/dto/rfid_scan_dto.dart';

class RfidScanRepository {
  Future<RfidScan> readRfidScannedBySeq ({
    required int sessionSeq,
  }) async {
    var provider = RfidScanProvider();
    RfidScan? result = await provider.readRfidScannedBySeq(sessionSeq: sessionSeq);

    if (result == null) {
      throw Exception();
    }

    print(result.toString());

    return result;
  }

  Future<RfidAllData> readAllRfidScannedDataBySeq ({
    required int sessionSeq,
  }) async {
    var provider = RfidScanProvider();
    RfidAllData? result = await provider.readAllRfidScannedDataBySeq(sessionSeq: sessionSeq);

    if (result == null) {
      throw Exception();
    }

    return result;
  }

  Future<bool> createRfidScan ({
    required String qrCode,
    required int sessionSeq,
  }) async {
    var provider = RfidScanProvider();
    bool? result = await provider.createRfidScan(qrCode: qrCode, sessionSeq: sessionSeq);

    return result;
  }
}