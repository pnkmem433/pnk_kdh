class CommonResponseDTO {
  int? fieldCount;
  int? affectedRows;
  int? insertId;
  int? serverStatus;
  int? warningCount;
  String? message;
  bool? protocol41;
  int? changedRows;

  CommonResponseDTO({
    this.fieldCount,
    this.affectedRows,
    this.insertId,
    this.serverStatus,
    this.warningCount,
    this.message,
    this.protocol41,
    this.changedRows,
  });

  factory CommonResponseDTO.fromJson(Map<String, dynamic> json) => CommonResponseDTO(
    fieldCount: json["fieldCount"],
    affectedRows: json["affectedRows"],
    insertId: json["insertId"],
    serverStatus: json["serverStatus"],
    warningCount: json["warningCount"],
    message: json["message"],
    protocol41: json["protocol41"],
    changedRows: json["changedRows"],
  );

  Map<String, dynamic> toJson() => {
    "fieldCount": fieldCount,
    "affectedRows": affectedRows,
    "insertId": insertId,
    "serverStatus": serverStatus,
    "warningCount": warningCount,
    "message": message,
    "protocol41": protocol41,
    "changedRows": changedRows,
  };
}
