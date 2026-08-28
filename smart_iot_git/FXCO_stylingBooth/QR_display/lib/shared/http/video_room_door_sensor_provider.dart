import 'package:fxco_stylingbooth/shared/models/dto/rfid_scan_dto.dart';
import 'package:http/http.dart' as http;

import '../../core/config/database_table.dart';
import '../models/dto/video_room_door_sensor_dto.dart';

class VideoRoomDoorSensorProvider {
  Future<VideoRoomDoorSensorLast?> readVideoRoomDoorSensorLastBySeq({
    required int sessionSeq,
  }) async {
    var url = Uri.parse(DatabaseTable.readVideoRoomDoorSensorLastBySeq+sessionSeq.toString());

    try {
      var response = await http.get(url);
      if (response.statusCode == 200) {
        VideoRoomDoorSensorLast result = videoRoomDoorSensorLastFromJson(response.body);
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