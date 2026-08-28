import 'package:fxco_stylingbooth/shared/http/video_room_door_sensor_provider.dart';
import 'package:fxco_stylingbooth/shared/models/dto/fitting_room_door_sensor_dto.dart';
import 'package:fxco_stylingbooth/shared/models/dto/rfid_scan_dto.dart';
import 'package:http/http.dart' as http;

import '../../core/config/database_table.dart';
import '../models/dto/video_room_door_sensor_dto.dart';
import 'fitting_room_door_sensor_provider.dart';

class FittingRoomDoorSensorRepository {
  Future<FittingRoomDoorSensor> readFittingRoomDoorSensorLatestOne () async {
    var provider = FittingRoomDoorSensorProvider();
    FittingRoomDoorSensor? result = await provider.readFittingRoomDoorSensorLatestOne();

    if (result == null) {
      throw Exception();
    }

//    print("FittingRoomDoorSensorRepository > readFittingRoomDoorSensorLatestOne");
    return result;
  }
}