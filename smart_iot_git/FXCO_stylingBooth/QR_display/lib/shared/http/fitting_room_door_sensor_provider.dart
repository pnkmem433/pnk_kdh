import 'package:fxco_stylingbooth/shared/models/dto/rfid_scan_dto.dart';
import 'package:http/http.dart' as http;

import '../../core/config/database_table.dart';
import '../models/dto/fitting_room_door_sensor_dto.dart';
import '../models/dto/video_room_door_sensor_dto.dart';

class FittingRoomDoorSensorProvider {
  Future<FittingRoomDoorSensor?> readFittingRoomDoorSensorLatestOne() async {
    var url = Uri.parse(DatabaseTable.readFittingRoomDoorSensorLatestOne);

    try {
      var response = await http.get(url);
      if (response.statusCode == 200) {
        FittingRoomDoorSensor result = fittingRoomDoorStateFromJson(response.body);
//        print("readFittingRoomDoorSensorLatestOne: $result");
        return result;
      } else {
        throw Exception('Failed to readRfidScannedBySeq');
      }
    } catch (e) {
//      print("오류 발생: $e");
      return null;
    }
  }
}