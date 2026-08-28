import 'dart:convert';

import 'package:fxco_stylingbooth/shared/http/system_parameter_provider.dart';
import 'package:http/http.dart' as http;

import '../../core/config/database_table.dart';
import '../models/dto/session_dto.dart';
import '../models/dto/system_parameter_dto.dart';

class SystemParameterRepository {
  Future<int> getSystemParameterBySeq ({
    required int seq,
  }) async {
    var provider = SystemParameterProvider();
    SystemParameter? result = await provider.getSystemParameterBySeq(seq: seq);

    if (result == null) {
      throw Exception();
    }

    print(result.toString());

    return result.parameter;
  }
}