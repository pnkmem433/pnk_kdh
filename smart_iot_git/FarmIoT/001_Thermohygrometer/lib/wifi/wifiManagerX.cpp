#include "wifiManagerX.h"

#include <ESP32Ping.h>

WifiManagerX::WifiManagerX(wifiManagerXParams wifiManagerXParams) {
  isNeedsInternet = wifiManagerXParams.isNeedsInternet;
  if (wifiManagerXParams.timeout > 0) {
    timeout = wifiManagerXParams.timeout;
  }

  permanentCredential = wifiManagerXParams.rootWiFi;
  wifiStatus = WifiStatus::none;

  // rootWiFi가 유효한지 체크 (SSID가 비어있지 않으면 유효한 것으로 간주)
  if (!wifiManagerXParams.rootWiFi.ssid.isEmpty()) {
    hasPermanentCredential = true;
    credentials.push_back(
        wifiManagerXParams.rootWiFi); // rootWiFi가 유효할 때만 리스트에 추가
  } else {
    hasPermanentCredential = false; // rootWiFi가 없으면 설정하지 않음
  }
}

void WifiManagerX::set(std::vector<WifiCredential> wifiCredentials) {
  for (const auto &wiFiCredential : wifiCredentials) {
    // 필수 Wi-Fi 정보 변경 방지
    if (hasPermanentCredential &&
        wiFiCredential.ssid == permanentCredential.ssid) {
      Serial.println("Cannot update permanent Wi-Fi: " + wiFiCredential.ssid);
      continue; // 필수 Wi-Fi는 건너뛰고 다음 Wi-Fi 추가 진행
    }

    bool updated = false;
    for (auto &cred : credentials) {
      if (cred.ssid == wiFiCredential.ssid) {
        cred.password = wiFiCredential.password;
        Serial.println("Password updated for SSID: " + cred.ssid);
        updated = true;
        break;
      }
    }

    // 기존 SSID가 없으면 추가
    if (!updated) {
      credentials.push_back(wiFiCredential);
      Serial.println("New WiFi added: " + String(wiFiCredential.ssid));
    }
  }
}

void WifiManagerX::remove(String ssid) {
  // 필수 Wi-Fi 삭제 방지
  if (hasPermanentCredential && String(ssid) == permanentCredential.ssid) {
    Serial.println("Cannot remove permanent Wi-Fi: " +
                   permanentCredential.ssid);
    return;
  }

  for (auto it = credentials.begin(); it != credentials.end(); ++it) {
    if (it->ssid == ssid) {
      credentials.erase(it);
      Serial.println("WiFi removed: " + String(ssid));
      return;
    }
  }
  Serial.println("SSID not found: " + String(ssid));
}

void WifiManagerX::reset() {
  if (hasPermanentCredential) {
    credentials.clear();
    wifiStatus = WifiStatus::none;
    credentials.push_back(permanentCredential); // 필수 Wi-Fi 유지
    Serial.println("WiFi credentials reset, but permanent Wi-Fi preserved.");
  } else {
    credentials.clear();
    Serial.println("All WiFi credentials have been reset.");
  }
}

std::vector<String> WifiManagerX::getSavedCredentials() {
  std::vector<String> savedSsidList;

  // 각 WifiCredential에서 SSID만 추출하여 추가
  for (const auto &credential : credentials) {
    savedSsidList.push_back(credential.ssid);
  }

  return savedSsidList;
}

void WifiManagerX::connect() {
  wifiStatus = WifiStatus::connecting;
  unsigned long start = millis();
  std::vector<int> availableIndexes;
  std::vector<int> availableRssiIndexes;

  Serial.println("\n\nConnecting to WiFi...");

  // 와이파이가 이미 연결되어 있는 경우 종료
  Serial.println("\n1. checking networks connection...");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("error: Already connected to: " + WiFi.SSID());
    wifiStatus = WifiStatus::connected;
    return;
  }

  // 주변 와이파이 스캔
  Serial.print("\n2. finding Available networks");
  WiFi.scanNetworks(true); // 비동기 스캔 시작

  unsigned long startScan = millis();
  const unsigned long timeout = 5000; // 최대 5초 대기

  while (WiFi.scanComplete() < 0) {
    if (millis() - startScan > timeout) {
      Serial.println("\nerror: Wi-Fi scan timeout");
      wifiStatus = WifiStatus::none;
      WiFi.scanDelete(); // 실패한 스캔 결과 정리
      return;
    }
    Serial.print(".");
    delay(500);
  }

  Serial.println("scan done.");
  int networkCount = WiFi.scanComplete();

  if (networkCount == 0) {
    Serial.println("error: No networks found.");
    wifiStatus = WifiStatus::none;
    return;
  }

  // 사용 가능한 네트워크 찾기
  for (int i = 0; i < networkCount; i++) {
    int rssi = WiFi.RSSI(i);
    String detectedSSID = WiFi.SSID(i);
    Serial.println(" - " + detectedSSID + " (" + rssi + " dBm)");

    for (size_t j = 0; j < credentials.size(); j++) {
      if (credentials[j].ssid == detectedSSID) {
        if (detectedSSID == permanentCredential.ssid) {
          availableIndexes.insert(availableIndexes.begin(), j);
          availableRssiIndexes.insert(availableRssiIndexes.begin(), i);
        } else {
          availableIndexes.push_back(j);
          availableRssiIndexes.push_back(i);
        }
      }
    }
  }

  // 사용 가능한 네트워크가 없으면 종료
  if (availableIndexes.empty()) {
    Serial.println("error: No matching networks found.");
    wifiStatus = WifiStatus::none;
    return;
  }

  Serial.print("\n3. List of Wi-Fi networks to attempt to connect");
  Serial.println("(Root Wi-Fi first, followed by strongest signal strength)");
  for (int i = 0; i < availableIndexes.size(); i++) {
    String detectedSSID = credentials[availableIndexes[i]].ssid;
    int rssi = WiFi.RSSI(availableRssiIndexes[i]);

    Serial.println(" - " + detectedSSID + " (" + rssi + " dBm)");
  }

  // 신호 강도를 기준으로 다른 네트워크에 연결 시도
  Serial.println("\n4. Connecting to the best available network...");
  for (int i = 0; i < availableIndexes.size(); i++) {
    int idx = availableIndexes[i];
    credentials[idx].password =
        credentials[idx].password.isEmpty() ? "" : credentials[idx].password;
    if (connect(credentials[idx])) {
      Serial.println("\nConnected to \"" + credentials[idx].ssid + "\" in " +
                     String(millis() - start) + "ms");
      wifiStatus = WifiStatus::connected;
      return;
    }
  }

  Serial.println("error: All available networks failed to connect.");
  wifiStatus = WifiStatus::none;
}

bool WifiManagerX::connect(WifiCredential wifiCredential) {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("error: Already connected to: " + WiFi.SSID());
    wifiStatus = WifiStatus::connected;
    return true;
  }

  wifiStatus = WifiStatus::connecting;
  Serial.print(" - " + wifiCredential.ssid);
  WiFi.begin(wifiCredential.ssid.c_str(), wifiCredential.password.c_str());

  unsigned long attempts = millis();
  while ((WiFi.status() != WL_CONNECTED) && (millis() - attempts <= timeout)) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("connected, checking internet connection...");

    // 인터넷 확인 (필요한 경우만)
    if (!isNeedsInternet) {
      Serial.println("...Internet is not required.");
      wifiStatus = WifiStatus::connected;
      return true;
    } else if (Ping.ping("8.8.8.8") >= 0) {
      Serial.println("...Internet is available.");
      wifiStatus = WifiStatus::connected;
      return true;
    } else {
      Serial.println("...No internet. Trying next network.");
    }
  } else {
    Serial.println("Failed to connect. Trying next network.");
  }

  wifiStatus = WifiStatus::none;
  return false;
}

void WifiManagerX::disconnect() {
  WiFi.disconnect();
  Serial.println("WiFi Disconnected.");
}

WifiStatus WifiManagerX::getWifiStatus() {
  if (WiFi.status() != WL_CONNECTED && wifiStatus != WifiStatus::connecting) {
    return WifiStatus::none;
  }
  return wifiStatus;
}

String WifiManagerX::connectssid() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.SSID();
  }
  return "Not connected";
}

std::vector<String> WifiManagerX::scanWifi() {
  std::vector<String> scanWifis;

  // 비동기 스캔 시작
  WiFi.scanNetworks(true);

  unsigned long start = millis();
  const unsigned long timeout = 5000; // 5초

  int networkCount = -1;
  while ((millis() - start) < timeout) {
    networkCount = WiFi.scanComplete();
    if (networkCount >= 0) {
      break;
    }
    delay(100);
  }

  // 스캔 실패 또는 타임아웃
  if (networkCount < 0) {
    WiFi.scanDelete(); // 리소스 정리
    return {};
  }

  for (int i = 0; i < networkCount; i++) {
    int rssi = WiFi.RSSI(i);
    String detectedSSID = WiFi.SSID(i);
    scanWifis.push_back(String(rssi) + ":" + detectedSSID + ";");
  }

  return scanWifis;
}
