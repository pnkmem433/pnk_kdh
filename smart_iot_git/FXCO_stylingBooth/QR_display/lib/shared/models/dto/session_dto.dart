import 'dart:convert';

Session sessionFromJson(String str) => Session.fromJson(json.decode(str));

String sessionToJson(Session data) => json.encode(data.toJson());

class Session {
  int seq;
  DateTime createdAt;
  int isScanned;
  int isVideoEnded;
  int isActivated;
  int? pirStatus;

  Session({
    required this.seq,
    required this.createdAt,
    required this.isScanned,
    required this.isVideoEnded,
    required this.isActivated,
    this.pirStatus,
  });

  factory Session.fromJson(Map<String, dynamic> json) => Session(
    seq: json["seq"],
    createdAt: DateTime.parse(json["created_at"]),
    isScanned: json["is_scanned"],
    isVideoEnded: json["is_video_ended"],
    isActivated: json["is_activated"],
    pirStatus: json["pir_status"],
  );

  Map<String, dynamic> toJson() => {
    "seq": seq,
    "created_at": createdAt.toIso8601String(),
    "is_scanned": isScanned,
    "is_video_ended": isVideoEnded,
    "is_activated": isActivated,
    "pir_status": pirStatus,
  };
}