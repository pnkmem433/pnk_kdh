import 'dart:convert';
import 'package:get/get.dart';
import 'package:http/http.dart' as http;

class ApiService extends GetxService {
  static const String _baseUrl = 'http://192.168.1.67:3002';
  
  Future<bool> checkHanger(String qrCode) async {
    print("[API] checkHanger: $qrCode");
    await Future.delayed(const Duration(milliseconds: 500));
    // Simulate valid if not empty
    return qrCode.isNotEmpty;
  }

  Future<bool> linkClothes(String hangerQr, String clothesId) async {
    print("[API] linkClothes: Hanger=$hangerQr, Clothes=$clothesId");
    await Future.delayed(const Duration(milliseconds: 800));
    return true;
  }

  Future<int?> matchClothes({
    required String hangerUuid,
    required int clothesSeq,
  }) async {
    final uri = Uri.parse('$_baseUrl/hanger/match-clothes');
    final body = <String, dynamic>{
      "hangerUuid": hangerUuid,
      "clothesSeq": clothesSeq,
    };

    try {
      print("[HTTP PATCH] $uri");
      print(body);

      final res = await http.patch(
        uri,
        headers: const {
          "Content-Type": "application/json",
        },
        body: jsonEncode(body),
      );

      final ok = res.statusCode >= 200 && res.statusCode < 300;
      if (!ok) {
        print("[HTTP PATCH] Failed: ${res.statusCode} ${res.body}");
      }
      return res.statusCode;
    } catch (e) {
      print("[HTTP PATCH] Exception: $e");
      return null;
    }
  }

  Future<bool> sendAssociation(String hangerQr, String hangerRackUuid) async {
    print("[API] sendAssociation: Hanger=$hangerQr, Rack=$hangerRackUuid");
    await Future.delayed(const Duration(milliseconds: 800));
    return true;
  }
  
  Future<bool> saveRackPair(String hangerRackUuid, String rackUuid) async {
    print("[API] saveRackPair: SmartRack=$hangerRackUuid, Rack=$rackUuid");
    await Future.delayed(const Duration(milliseconds: 800));
    return true;
  }

  Future<bool> replaceHangerLeg({
    required String hangerlegUuid,
    required int rackSeq,
    required int position,
  }) async {
    final uri = Uri.parse('$_baseUrl/hangerleg/replace');
    final body = <String, dynamic>{
      "hangerlegUuid": hangerlegUuid,
      "rackSeq": rackSeq,
      "position": position,
    };

    try {
      print("[HTTP PATCH] $uri");
      print(body);

      final res = await http.patch(
        uri,
        headers: const {
          "Content-Type": "application/json",
        },
        body: jsonEncode(body),
      );

      final ok = res.statusCode >= 200 && res.statusCode < 300;
      if (!ok) {
        print("[HTTP PATCH] Failed: ${res.statusCode} ${res.body}");
      }
      return ok;
    } catch (e) {
      print("[HTTP PATCH] Exception: $e");
      return false;
    }
  }

  Future<bool> replaceHanger({
    required String hangerUuid,
    required String hangerlegUuid,
  }) async {
    final uri = Uri.parse('$_baseUrl/hanger/replace');
    final body = <String, dynamic>{
      "hangerUuid": hangerUuid,
      "hangerlegUuid": hangerlegUuid,
    };

    try {
      print("[HTTP PATCH] $uri");
      print(body);

      final res = await http.patch(
        uri,
        headers: const {
          "Content-Type": "application/json",
        },
        body: jsonEncode(body),
      );

      final ok = res.statusCode >= 200 && res.statusCode < 300;
      if (!ok) {
        print("[HTTP PATCH] Failed: ${res.statusCode} ${res.body}");
      }
      return ok;
    } catch (e) {
      print("[HTTP PATCH] Exception: $e");
      return false;
    }
  }
}
