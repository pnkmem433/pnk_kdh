import 'dart:convert';

SystemParameter systemParameterFromJson(String str) => SystemParameter.fromJson(json.decode(str));

String systemParameterToJson(SystemParameter data) => json.encode(data.toJson());

class SystemParameter {
  int seq;
  String paramName;
  int parameter;

  SystemParameter({
    required this.seq,
    required this.paramName,
    required this.parameter,
  });

  factory SystemParameter.fromJson(Map<String, dynamic> json) => SystemParameter(
    seq: json["seq"],
    paramName: json["param_name"],
    parameter: json["parameter"],
  );

  Map<String, dynamic> toJson() => {
    "seq": seq,
    "param_name": paramName,
    "parameter": parameter,
  };
}
