import 'dart:convert';

import 'package:fxco_stylingbooth/shared/models/dto/session_dto.dart';

SessionTrack sessionTrackFromJson(String str) => SessionTrack.fromJson(json.decode(str));

String sessionTrackToJson(SessionTrack data) => json.encode(data.toJson());

class SessionTrack {
  bool noSessionFlag;
  Session session;
  FittingRoomDoorClosed fittingRoomDoorClosed;
  TimeComparison timeComparison;
  bool pirSensorIssue;

  SessionTrack({
    required this.noSessionFlag,
    required this.session,
    required this.fittingRoomDoorClosed,
    required this.timeComparison,
    required this.pirSensorIssue,
  });

  factory SessionTrack.fromJson(Map<String, dynamic> json) => SessionTrack(
    noSessionFlag: json["noSessionFlag"],
    session: Session.fromJson(json["session"]),
    fittingRoomDoorClosed: FittingRoomDoorClosed.fromJson(json["fittingRoomDoorClosed"]),
    timeComparison: TimeComparison.fromJson(json["timeComparison"]),
    pirSensorIssue: json["pirSensorIssue"],
  );

  Map<String, dynamic> toJson() => {
    "noSessionFlag": noSessionFlag,
    "session": session.toJson(),
    "fittingRoomDoorClosed": fittingRoomDoorClosed.toJson(),
    "timeComparison": timeComparison.toJson(),
    "pirSensorIssue": pirSensorIssue,
  };
}

class FittingRoomDoorClosed {
  int seq;
  DateTime eventTime;
  int isOpened;

  FittingRoomDoorClosed({
    required this.seq,
    required this.eventTime,
    required this.isOpened,
  });

  factory FittingRoomDoorClosed.fromJson(Map<String, dynamic> json) => FittingRoomDoorClosed(
    seq: json["seq"],
    eventTime: DateTime.parse(json["event_time"]),
    isOpened: json["is_opened"],
  );

  Map<String, dynamic> toJson() => {
    "seq": seq,
    "event_time": eventTime.toIso8601String(),
    "is_opened": isOpened,
  };
}

class TimeComparison {
  DateTime sessionCreatedAt;
  DateTime doorClosedAt;
  int timeDiffMs;
  int timeDiffSeconds;
  bool isWithin5Seconds;

  TimeComparison({
    required this.sessionCreatedAt,
    required this.doorClosedAt,
    required this.timeDiffMs,
    required this.timeDiffSeconds,
    required this.isWithin5Seconds,
  });

  factory TimeComparison.fromJson(Map<String, dynamic> json) => TimeComparison(
    sessionCreatedAt: DateTime.parse(json["sessionCreatedAt"]),
    doorClosedAt: DateTime.parse(json["doorClosedAt"]),
    timeDiffMs: json["timeDiffMs"],
    timeDiffSeconds: json["timeDiffSeconds"],
    isWithin5Seconds: json["isWithin5Seconds"],
  );

  Map<String, dynamic> toJson() => {
    "sessionCreatedAt": sessionCreatedAt.toIso8601String(),
    "doorClosedAt": doorClosedAt.toIso8601String(),
    "timeDiffMs": timeDiffMs,
    "timeDiffSeconds": timeDiffSeconds,
    "isWithin5Seconds": isWithin5Seconds,
  };
}
