# MQTT Subscription Failure Fix (QR Scan)

## Problem Summary
MQTT subscription was failing when initiated from a QR code scan, despite working with a manual connection button. The analysis revealed that scanned QR codes often contained invisible whitespace or newline characters, which caused a string mismatch in the subscription confirmation logic.

## Changes Made

### 1. Trim Scanned QR Code
Modified `smart_hanger_controller.dart` to trim any leading or trailing whitespace from the QR code result before using it for registration or MQTT subscription.

```diff
   Future<void> startScan() async {
-    final result = await _scan.scanQr();
+    var result = await _scan.scanQr();
     if (result != null && result.isNotEmpty) {
+      result = result.trim(); // Trim whitespace/newlines
       statusMessage.value = "QR 확인 중...";
```

### 2. Improved Diagnostics
Updated `mqtt_service.dart` to log confirmed topics within quotes to make invisible characters visible in the logs.

```diff
   void _onSubscribed(String topic) {
-    print('[MQTT] Subscription confirmed for topic $topic');
+    print('[MQTT] Subscription confirmed for topic: "$topic"');
    subscribedTopic.value = topic;
    isSubscribed.value = true;
  }
```

## Verification

### Logical Verification
- [x] The `result` is trimmed immediately after scanning.
- [x] The subscription topic is built using the trimmed string.
- [x] The internal `_waitForSubscription` check now compares against the trimmed string.
- [x] Logging now clearly shows if any non-printable characters are present in the confirmed topic name.

### Manual Verification Required
1. Navigate to the **Smart Hanger** tab.
2. Click **QR코드 스캔하기**.
3. Scan a valid QR code (or use the Windows mock dialog and include a space/newline).
4. Verify that the **"MQTT 구독 성공"** snackbar appears.
5. Confirm the status chip shows **"토픽 구독됨"**.
