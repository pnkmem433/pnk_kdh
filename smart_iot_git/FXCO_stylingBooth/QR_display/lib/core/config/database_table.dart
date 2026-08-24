class DatabaseTable {
  DatabaseTable._();
//  static const webLink = 'http://115.23.192.217:3030';
  static const webLink = 'http://192.168.1.100:3000';

  static var sessionLast = "$webLink/session/last";
  static var readFittingRoomDoorSensorLatestOne = "$webLink/fitting-room-door-sensor/latest/one";
  static var updateIsScannedSession = "$webLink/session/is-scanned/";
  static var rfidScannedBySessionSeq = "$webLink/rfid-scan/last/record/";
  static var createRfidScan = "$webLink/rfid-scan";
  static var readAllRfidScannedDataBySeq = "$webLink/rfid-scan/last/record/all/";
  static var readVideoRoomDoorSensorLastBySeq = "$webLink/video-room-door-sensor/last/record/";
  static var readSystemParameterBySeq = "$webLink/system-parameter/";
  static var readSessionUntracked = "$webLink/session/untracked";
}