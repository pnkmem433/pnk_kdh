import 'dart:convert';

RfidScan rfidScanFromJson(String str) => RfidScan.fromJson(json.decode(str));

String rfidScanToJson(RfidScan data) => json.encode(data.toJson());

class RfidScan {
  int seq;
  int sessionSeq;
  String clothesProductId;
  DateTime scannedAt;
  int scanSourceType;

  RfidScan({
    required this.seq,
    required this.sessionSeq,
    required this.clothesProductId,
    required this.scannedAt,
    required this.scanSourceType,
  });

  factory RfidScan.fromJson(Map<String, dynamic> json) => RfidScan(
    seq: json["seq"],
    sessionSeq: json["session_seq"],
    clothesProductId: json["clothes_product_id"],
    scannedAt: DateTime.parse(json["scanned_at"]),
    scanSourceType: json["scan_source_type"],
  );

  Map<String, dynamic> toJson() => {
    "seq": seq,
    "session_seq": sessionSeq,
    "clothes_product_id": clothesProductId,
    "scanned_at": scannedAt.toIso8601String(),
    "scan_source_type": scanSourceType,
  };

  @override
  String toString() {
    // TODO: implement toString
    return "RfidScan("
        "seq: $seq, "
        "sessionSeq: $sessionSeq, "
        "clothesProductId: $clothesProductId, "
        "scannedAt: $scannedAt, "
        "scanSourceType: $scanSourceType"
        ")";
  }
}
