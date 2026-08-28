import 'dart:convert';

import 'package:http/http.dart' as http;

import '../../core/config/database_table.dart';
import '../models/dto/session_dto.dart';
import '../models/dto/session_track_dto.dart';

class SessionProvider {
  Future<Session?> getLastSession() async {
    var url = Uri.parse(DatabaseTable.sessionLast);

    try {
      var response = await http.get(url);
      if (response.statusCode == 200) {
        Session result = sessionFromJson(response.body);
//        print("getLastSession: $result");
        return result;
      } else {
        throw Exception('Failed to getLastSession');
      }
    } catch (e) {
      print("SessionProvider > getLastSession 오류 발생: $e");
      return null;
    }
  }

  Future<bool> updateIsScannedSession({
    required int sessionSeq,
  }) async {
    var url = Uri.parse(DatabaseTable.updateIsScannedSession+sessionSeq.toString());

    try {
      // PATCH 요청으로 JSON body 전송
      var response = await http.patch(
        url,
        headers: {'Content-Type': 'application/json'},
        body: jsonEncode({"value": 1}),
      );

      if (response.statusCode == 200) {
        return true;
      } else {
        return false;
      }
    } catch (e) {
      print("SessionProvider > updateIsScannedSession 오류 발생: $e");
      return false;
    }
  }

  Future<bool> readSessionUntracked() async {
    var url = Uri.parse(DatabaseTable.readSessionUntracked);

    try {
      var response = await http.get(url);
      if (response.statusCode == 200) {
        bool result = sessionTrackFromJson(response.body).noSessionFlag;
        return result;
      } else {
        return false;
      }
    } catch (e) {
      print("SessionProvider > readSessionUntracked 오류 발생: $e");
      return false;
    }
  }
}