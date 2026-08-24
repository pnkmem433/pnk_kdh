import 'dart:convert';

import 'package:http/http.dart' as http;

import '../../core/config/database_table.dart';
import '../models/dto/session_dto.dart';
import '../models/dto/system_parameter_dto.dart';

class SystemParameterProvider {
  Future<SystemParameter?> getSystemParameterBySeq({
    required int seq,
  }) async {
    var url = Uri.parse(DatabaseTable.readSystemParameterBySeq+seq.toString());

    try {
      var response = await http.get(url);
      if (response.statusCode == 200) {
        SystemParameter result = systemParameterFromJson(response.body);
        return result;
      } else {
        throw Exception('Failed to getLastSession');
      }
    } catch (e) {
      print("오류 발생: $e");
      return null;
    }
  }
}