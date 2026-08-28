import 'package:fxco_stylingbooth/shared/http/session_provider.dart';

import '../models/dto/session_dto.dart';

class SessionRepository {
  Future<int?> getLastSession () async {
    var provider = SessionProvider();
    Session? result = await provider.getLastSession();

    if (result == null) {
      return null;
    }

    if (result.isScanned == 0 && result.isVideoEnded == 0 && result.isActivated == 1) {
      return result.seq;
    } else {
      return null;
    }
  }

  Future<bool> isSessionEnded () async {
    var provider = SessionProvider();
    Session? result = await provider.getLastSession();

    if (result == null) {
      return false;
    }

    if (/*result.isScanned == 1 && result.isVideoEnded == 1 &&*/ result.isActivated == 0) {
      return true;
    } else {
      return false;
    }
  }

  Future<bool> updateIsScannedSession ({
    required int sessionSeq,
  }) async {
    var provider = SessionProvider();
    bool result = await provider.updateIsScannedSession(sessionSeq: sessionSeq);
    return result;
  }

  Future<bool> readSessionUntracked () async {
    var provider = SessionProvider();
    bool result = await provider.readSessionUntracked();
    return result;
  }
}