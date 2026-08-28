import 'dart:convert';

VideoRoomDoorSensorLast videoRoomDoorSensorLastFromJson(String str) => VideoRoomDoorSensorLast.fromJson(json.decode(str));

String videoRoomDoorSensorLastToJson(VideoRoomDoorSensorLast data) => json.encode(data.toJson());

class VideoRoomDoorSensorLast {
  int seq;
  DateTime? videoStartTime;
  DateTime? videoEndTime;
  DateTime eventTime;
  int isOpened;

  VideoRoomDoorSensorLast({
    required this.seq,
    this.videoStartTime,
    this.videoEndTime,
    required this.eventTime,
    required this.isOpened,
  });

  factory VideoRoomDoorSensorLast.fromJson(Map<String, dynamic> json) => VideoRoomDoorSensorLast(
    seq: json["seq"],
    videoStartTime: json["video_start_time"] == null ? null : DateTime.parse(json["video_start_time"]),
    videoEndTime: json["video_end_time"] == null ? null : DateTime.parse(json["video_end_time"]),
    eventTime: DateTime.parse(json["event_time"]),
    isOpened: json["is_opened"],
  );

  Map<String, dynamic> toJson() => {
    "seq": seq,
    "video_start_time": videoStartTime?.toIso8601String(),
    "video_end_time": videoEndTime?.toIso8601String(),
    "event_time": eventTime.toIso8601String(),
    "is_opened": isOpened,
  };

  @override
  String toString() {
    // TODO: implement toString
    return "VideoRoomDoorSensorLast("
        "seq: $seq, "
        "videoStartTime: $videoStartTime, "
        "videoEndTime: $videoEndTime, "
        "eventTime: $eventTime, "
        "isOpened: $isOpened"
        ")";
  }
}
