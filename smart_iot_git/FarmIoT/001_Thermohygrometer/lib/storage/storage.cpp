#include "storage.h"

Storage::Storage() {
  _bleFileName = "/ble_config.txt";
  _wifiFileName = "/wifi_config.txt"; // 기본 파일 이름 설정
  _thermohygrometerDataFileName = "/thermohygrometerData.txt";

  _fsMutex = xSemaphoreCreateMutex(); // 뮤텍스 초기화
}

void Storage::begin() {
  if (!LittleFS.begin(true)) {
    Serial.println("Failed to mount LittleFS.");
  } else {
    Serial.println("LittleFS mounted successfully!");
  }
}

std::vector<String> Storage::loadBleUuids() {
  FS_LOCK(); // 파일 접근을 보호

  std::vector<String> bleUuids;

  File file = LittleFS.open(_bleFileName, "r");
  bool fileExists = file;

  if (fileExists) {
    while (file.available()) {
      String uuid = file.readStringUntil('\n');
      uuid.trim();
      if (uuid.length() > 0) {
        bleUuids.push_back(uuid);
      }
    }
    file.close();
  }

  // 파일이 없었거나, 읽은 UUID가 3개 미만이면 새로 생성
  if (!fileExists || bleUuids.size() < 3) {
    Serial.println("BLE UUID 새로 생성합니다.");

    File fileWrite = LittleFS.open(_bleFileName, "w");
    if (!fileWrite) {
      Serial.println("BLE UUID 파일 쓰기 실패");
      FS_UNLOCK();
      return bleUuids;
    }

    bleUuids.clear(); // 기존 읽은 거 무시하고 새로 시작

    for (int i = 0; i < 3; i++) {
      String uuid = generateRandomUuidV4();
      fileWrite.println(uuid);
      bleUuids.push_back(uuid);
    }

    fileWrite.close();
  }

  FS_UNLOCK(); // 파일 접근 해제
  return bleUuids;
}

bool Storage::addWifi(WifiCredential wifiCredential) {
  FS_LOCK(); // 파일 접근 보호

  String fileContent; // <- 함수 맨 처음에 선언

  // 파일 열기 (읽기 모드)
  File file = LittleFS.open(_wifiFileName, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
  } else {
    fileContent = file.readString(); // 파일 내용 읽기
    file.close();                    // 파일 닫기

    // 기존에 같은 ssid가 있는지 확인하고 있으면 password만 변경
    int startPos = fileContent.indexOf(wifiCredential.ssid + ";");
    if (startPos != -1) { // ssid가 이미 존재하는 경우
      int endPos =
          fileContent.indexOf("\n", startPos); // 해당 라인의 끝 위치 찾기
      if (endPos != -1) {
        String oldEntry =
            fileContent.substring(startPos, endPos + 1); // 기존 entry
        String newEntry = wifiCredential.ssid + ";" + wifiCredential.password +
                          "\n";                  // 새 entry
        fileContent.replace(oldEntry, newEntry); // 기존 entry를 새 entry로 교체
      }
    } else {
      // ssid가 없으면 새로운 entry 추가
      String newEntry =
          wifiCredential.ssid + ";" + wifiCredential.password + "\n";
      fileContent += newEntry; // 기존 내용에 새 Wi-Fi credential 추가
    }
  }

  // 파일 열기 (쓰기 모드)
  file = LittleFS.open(_wifiFileName, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    FS_UNLOCK(); // 파일 접근 해제
    return false;
  }

  file.print(fileContent); // 변경된 내용 파일에 기록
  file.close();
  Serial.println("Wi-Fi credential added or updated in file.");

  FS_UNLOCK(); // 파일 접근 해제
  return true;
}

bool Storage::resetWiFi() {
  FS_LOCK(); // 파일 접근 보호

  bool result = LittleFS.remove(_wifiFileName);
  FS_UNLOCK(); // 파일 접근 해제

  if (result) {
    Serial.println("Wi-Fi credentials reset successfully.");
  } else {
    Serial.println("Failed to reset Wi-Fi credentials.");
  }
  return result;
}

std::vector<WifiCredential> Storage::loadWifis() {
  FS_LOCK(); // 파일 접근 보호

  // 파일 열기 (읽기 모드)
  File file = LittleFS.open(_wifiFileName, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    FS_UNLOCK(); // 파일 접근 해제
    return {};
  }

  String fileContent = file.readString(); // 파일 내용 읽기
  file.close();

  std::vector<WifiCredential> credentialsList;

  // 파일 내용을 한 줄씩 처리
  int startIdx = 0;
  int endIdx = fileContent.indexOf('\n', startIdx);
  while (endIdx != -1) {
    String line = fileContent.substring(startIdx, endIdx);
    int separatorIdx = line.indexOf(';');

    if (separatorIdx != -1) {
      WifiCredential cred;
      cred.ssid = line.substring(0, separatorIdx);
      cred.password = line.substring(separatorIdx + 1);
      credentialsList.push_back(cred);
    }

    startIdx = endIdx + 1;
    endIdx = fileContent.indexOf('\n', startIdx);
  }

  FS_UNLOCK(); // 파일 접근 해제
  return credentialsList;
}

// 온습도 데이터 추가 함수
void Storage::addThermohygrometerData(ThermohygrometerData data) {
  FS_LOCK(); // 파일 접근 보호

  if (!LittleFS.exists(_thermohygrometerDataFileName)) {
    File file = LittleFS.open(_thermohygrometerDataFileName, FILE_WRITE);
    if (!file) {
      Serial.println("온습도 데이터 파일 생성 실패.");
      FS_UNLOCK(); // 파일 접근 해제
      return;
    }
    file.close();
  }

  // 파일 열기
  File file = LittleFS.open(_thermohygrometerDataFileName, FILE_APPEND);
  if (!file) {
    Serial.println("온습도 데이터 파일 열기 실패.");
    FS_UNLOCK(); // 파일 접근 해제
    return;
  }

  // 파일 크기가 제한을 초과한 경우 오래된 데이터를 삭제
  if (LittleFS.totalBytes() - LittleFS.usedBytes() <= 1000) {
    FS_UNLOCK();     // 파일 접근 해제
    file.close();    // 기존 파일을 닫고
    deleteOldData(); // 오래된 데이터 삭제 함수 호출

    file = LittleFS.open(_thermohygrometerDataFileName,
                         FILE_APPEND); // 다시 파일 열기
    if (!file) {
      Serial.println("파일 열기 실패.");
      FS_UNLOCK(); // 파일 접근 해제
      return;
    }
  }

  // 시간, 온도, 습도를 "{시간}:{온도}:{습도}" 형식으로 파일에 저장
  file.println(data.dateTime.toString() + "\t" + String(data.temp) + "\t" +
               String(data.humi));
  file.close();

  FS_UNLOCK(); // 파일 접근 해제
}

void Storage::deleteOldData() {
  FS_LOCK(); // 파일 접근 보호

  // 파일을 읽기 모드로 열기
  File file = LittleFS.open(_thermohygrometerDataFileName, FILE_READ);
  if (!file) {
    Serial.println("온습도 데이터 파일 열기 실패.");
    FS_UNLOCK(); // 파일 접근 해제
    return;
  }

  String data = "";
  while (file.available()) {
    data += (char)file.read();
  }
  file.close();

  // 데이터에서 오래된 부분을 제거 (첫 번째 줄을 지우는 예시)
  int firstNewLineIndex = data.indexOf('\n');
  if (firstNewLineIndex != -1) {
    data = data.substring(firstNewLineIndex + 1); // 첫 번째 줄을 제거
  }

  // 파일을 덮어쓰기 모드로 열기
  file = LittleFS.open(_thermohygrometerDataFileName, FILE_WRITE);
  if (!file) {
    Serial.println("파일 열기 실패.");
    FS_UNLOCK(); // 파일 접근 해제
    return;
  }

  file.print(data); // 최신 데이터만 파일에 기록
  file.close();

  FS_UNLOCK(); // 파일 접근 해제
}

bool Storage::getThermohygrometerData(
    std::function<bool(std::vector<ThermohygrometerData>)> callbackFunction) {
  FS_LOCK(); // 파일 접근 보호

  if (!LittleFS.exists(_thermohygrometerDataFileName)) {
    FS_UNLOCK();
    return true;
  }

  File file = LittleFS.open(_thermohygrometerDataFileName, FILE_READ);
  if (!file) {
    Serial.println("온습도 데이터 파일 열기 실패.");
    FS_UNLOCK(); // 파일 접근 해제
    return false;
  }

  // 중단된 위치를 기록하기 위한 변수
  unsigned long lastProcessedPosition = 0;
  bool processingStopped = false;

  std::vector<ThermohygrometerData> dataBatch; // 데이터 100개를 저장할 벡터

  // 파일에서 온습도 데이터를 읽어와서 콜백 함수에 전달
  while (file.available()) {
    unsigned long currentPosition = file.position(); // 현재 위치 저장
    String line = file.readStringUntil('\n');
    int colon1 = line.indexOf('\t');
    int colon2 = line.indexOf('\t', colon1 + 1);

    if (colon1 != -1 && colon2 != -1) {
      ThermohygrometerData data;
      DateTime datetime = {0, 0, 0, 0, 0, 0};
      datetime.fromString(line.substring(0, colon1));
      data.dateTime = datetime;
      data.temp = line.substring(colon1 + 1, colon2).toFloat();
      data.humi = line.substring(colon2 + 1).toFloat();

      dataBatch.push_back(data);

      // 100개씩 모아서 콜백 함수 호출
      if (dataBatch.size() >= 120) {
        if (!callbackFunction(dataBatch)) {
          // 처리 중단 시, 현재 위치를 기록
          lastProcessedPosition = currentPosition;
          processingStopped = true;
          break;
        }
        dataBatch.clear(); // 데이터를 보낸 후, 배치 초기화
      }
    } else {
      resetThermohygrometerData();
      file.close();
      FS_UNLOCK(); // 파일 접근 해제
      return false;
    }
  }

  // 남은 데이터가 있다면 콜백 함수 호출
  if (!dataBatch.empty()) {
    if (!callbackFunction(dataBatch)) {
      lastProcessedPosition = file.position();
      processingStopped = true;
    }
  }

  file.close();

  // 중단된 위치 이후 데이터를 새로운 파일로 저장
  if (processingStopped) {
    File oldFile = LittleFS.open(_thermohygrometerDataFileName, FILE_READ);
    if (!oldFile) {
      Serial.println("기존 데이터 파일을 다시 열기 실패.");
      FS_UNLOCK(); // 파일 접근 해제
      return false;
    }

    File newFile = LittleFS.open("/temp_data.txt", FILE_WRITE);
    if (!newFile) {
      Serial.println("새로운 데이터 파일 생성 실패.");
      oldFile.close();
      FS_UNLOCK(); // 파일 접근 해제
      return false;
    }

    // 중단된 위치부터 끝까지 복사
    oldFile.seek(lastProcessedPosition);
    while (oldFile.available()) {
      String remainingLine = oldFile.readStringUntil('\n');
      newFile.println(remainingLine);
    }

    oldFile.close();
    newFile.close();

    // 기존 파일 삭제 및 새 파일로 교체
    LittleFS.remove(_thermohygrometerDataFileName);
    LittleFS.rename("/temp_data.txt", _thermohygrometerDataFileName);
  } else {
    // 파일을 다 읽었다면 삭제
    if (LittleFS.exists(_thermohygrometerDataFileName)) {
      LittleFS.remove(_thermohygrometerDataFileName);
      Serial.println("온습도 데이터 파일 삭제됨.");
    }
    FS_UNLOCK();
    return true;
  }

  FS_UNLOCK();
  return false;
}

// 온습도 데이터 초기화
bool Storage::resetThermohygrometerData() {
  FS_LOCK(); // 파일 접근 보호

  bool result = LittleFS.remove(_thermohygrometerDataFileName);
  FS_UNLOCK(); // 파일 접근 해제

  if (result) {
    Serial.println("온습도 데이터 초기화 성공.");
  } else {
    Serial.println("온습도 데이터 초기화 실패.");
  }
  return result;
}

String Storage::generateRandomUuidV4() {
  uint8_t uuidBytes[16];

  // 하드웨어 난수 생성
  for (int i = 0; i < 16; i += 4) {
    uint32_t r = esp_random(); // 4바이트 랜덤
    uuidBytes[i] = (r >> 24) & 0xFF;
    uuidBytes[i + 1] = (r >> 16) & 0xFF;
    uuidBytes[i + 2] = (r >> 8) & 0xFF;
    uuidBytes[i + 3] = r & 0xFF;
  }

  uuidBytes[6] = (uuidBytes[6] & 0x0F) | 0x40; // Version 4
  uuidBytes[8] = (uuidBytes[8] & 0x3F) | 0x80; // Variant

  char buf[37];
  sprintf(
      buf,
      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      uuidBytes[0], uuidBytes[1], uuidBytes[2], uuidBytes[3], uuidBytes[4],
      uuidBytes[5], uuidBytes[6], uuidBytes[7], uuidBytes[8], uuidBytes[9],
      uuidBytes[10], uuidBytes[11], uuidBytes[12], uuidBytes[13], uuidBytes[14],
      uuidBytes[15]);

  return String(buf);
}

void Storage::fileLock() {
  FS_LOCK();
  LittleFS.end();
}