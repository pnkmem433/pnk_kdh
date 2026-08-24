import 'package:fxco_stylingbooth/shared/http/video_room_door_sensor_provider.dart';
import 'package:fxco_stylingbooth/shared/models/dto/rfid_scan_dto.dart';
import 'package:http/http.dart' as http;

import '../../core/config/database_table.dart';
import '../models/dto/video_room_door_sensor_dto.dart';

class VideoRoomDoorSensorRepository {
  Future<VideoRoomDoorSensorLast> readRfidScannedBySeq ({
    required int sessionSeq,
  }) async {
    var provider = VideoRoomDoorSensorProvider();
    VideoRoomDoorSensorLast? result = await provider.readVideoRoomDoorSensorLastBySeq(sessionSeq: sessionSeq);

    if (result == null) {
      throw Exception();
    }

    return result;
  }
}