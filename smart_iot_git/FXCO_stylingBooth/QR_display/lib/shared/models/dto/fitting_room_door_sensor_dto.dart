import 'dart:convert';

FittingRoomDoorSensor fittingRoomDoorStateFromJson(String str) => FittingRoomDoorSensor.fromJson(json.decode(str));

String fittingRoomDoorStateToJson(FittingRoomDoorSensor data) => json.encode(data.toJson());

class FittingRoomDoorSensor {
  int seq;
  int sessionSeq;
  DateTime eventTime;
  int isOpened;

  FittingRoomDoorSensor({
    required this.seq,
    required this.sessionSeq,
    required this.eventTime,
    required this.isOpened,
  });

  factory FittingRoomDoorSensor.fromJson(Map<String, dynamic> json) => FittingRoomDoorSensor(
    seq: json["seq"],
    sessionSeq: json["session_seq"],
    eventTime: DateTime.parse(json["event_time"]),
    isOpened: json["is_opened"],
  );

  Map<String, dynamic> toJson() => {
    "seq": seq,
    "session_seq": sessionSeq,
    "event_time": eventTime.toIso8601String(),
    "is_opened": isOpened,
  };

  @override
  String toString() {
    // TODO: implement toString
    return "FittingRoomDoorSensor("
        "seq: $seq, "
        "sessionSeq: $sessionSeq, "
        "eventTime: $eventTime, "
        "isOpened: $isOpened"
        ")";
  }
}
