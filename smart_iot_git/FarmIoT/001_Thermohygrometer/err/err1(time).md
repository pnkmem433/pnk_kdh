# Modbus 음수 값 처리 수정 내역

## 문제 현상
XY-MD03 온습도 센서에서 음수 값이 정상적으로 처리되지 않고 큰 양수로 출력됨.

- 예: -1.0 → 6553.5

## 원인
- 센서는 signed 16bit(int16_t) 형식으로 값을 전송함
- 기존 구현에서는 uint16_t로 레지스터 값을 해석함
- 2’s complement로 표현된 음수가 unsigned로 처리되어 값이 왜곡됨

## 수정 내용

### 반환 타입 변경
```diff
- std::vector<uint16_t>
+ std::vector<int16_t>
레지스터 값 해석 방식 변경
diff
Copy code
- uint16_t val = (response[...] << 8) | response[...];
+ uint16_t raw = (response[...] << 8) | response[...];
+ int16_t val = static_cast<int16_t>(raw);
함수 시그니처
cpp
Copy code
std::vector<int16_t> XyMd03ThermoHygroSensor::read(
    uint16_t startAddr,
    uint16_t numRegs
);
결과
음수 온도 정상 처리

Modbus 센서 데이터 타입과 의미 일치

비정상적인 대형 수치 출력 문제 해결

사용 예
cpp
Copy code
auto regs = sensor.read(0x0000, 2);

float temperature = regs[0] / 10.0f;
float humidity    = regs[1] / 10.0f;