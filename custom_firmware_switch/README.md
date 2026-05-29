# custom_firmware_switch

커스텀 펌웨어용 빌드/배포 스위치 폴더입니다.

목표:

- ESP8685 커스텀 펌웨어 빌드
- ESP-02S 커스텀 펌웨어 빌드
- 빌드 산출물을 구글드라이브 `custom` 폴더로 복사
- 기존 자체제작 OTA 대시보드(`/versions/create`)에 자동 등록

현재 연결 경로:

- ESP8685 프로젝트: `C:\WS\vs_kdh\pnk_kdh\smart_plug\SmartPlug_internship\SmartPlug`
- ESP-02S 프로젝트: `C:\WS\vs_kdh\pnk_kdh\smart_plug\SmartPlug_internship\esp02s`

주의:

- 현재 ESP-02S 커스텀 프로젝트는 `platformio.ini`가 없으면 자동으로 건너뜁니다.
- 지금 단계에서는 공통 코드 동기화는 하지 않습니다.
- 나중에 필요하면 `shared/common`, `shared/targets/esp8685`, `shared/targets/esp02s` 구조로 확장하면 됩니다.

실행:

```powershell
cd C:\WS\vs_kdh\pnk_kdh\custom_firmware_switch
.\build_all.cmd
```

특정 타겟만:

```powershell
python .\build_all.py --target esp8685
python .\build_all.py --target esp02s
```
