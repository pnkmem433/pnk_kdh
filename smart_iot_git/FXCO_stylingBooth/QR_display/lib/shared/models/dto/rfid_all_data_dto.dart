import 'dart:convert';

RfidAllData rfidAllDataFromJson(String str) => RfidAllData.fromJson(json.decode(str));

String rfidAllDataToJson(RfidAllData data) => json.encode(data.toJson());

class RfidAllData {
  int seq;
  int sessionSeq;
  String clothesTypesName;
  String clothesProductId;
  String videoUrl;
  DateTime scannedAt;
  int scanSourceType;

  RfidAllData({
    required this.seq,
    required this.sessionSeq,
    required this.clothesTypesName,
    required this.clothesProductId,
    required this.videoUrl,
    required this.scannedAt,
    required this.scanSourceType,
  });

  factory RfidAllData.fromJson(Map<String, dynamic> json) => RfidAllData(
    seq: json["seq"],
    sessionSeq: json["session_seq"],
    clothesTypesName: json["clothes_types_name"],
    clothesProductId: json["clothes_product_id"],
    videoUrl: json["video_url"],
    scannedAt: DateTime.parse(json["scanned_at"]),
    scanSourceType: json["scan_source_type"],
  );

  Map<String, dynamic> toJson() => {
    "seq": seq,
    "session_seq": sessionSeq,
    "clothes_types_name": clothesTypesName,
    "clothes_product_id": clothesProductId,
    "video_url": videoUrl,
    "scanned_at": scannedAt.toIso8601String(),
    "scan_source_type": scanSourceType,
  };

  @override
  String toString() {
    // TODO: implement toString
    return "RfidAllData("
        "seq: $seq, "
        "sessionSeq: $sessionSeq, "
        "clothesTypesName: $clothesTypesName, "
        "clothesProductId: $clothesProductId, "
        "videoUrl: $videoUrl, "
        "scannedAt: $scannedAt, "
        "scanSourceType: $scanSourceType"
        ")";
  }
}
