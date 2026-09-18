# APNX_qt_monitor — User Space Packet Monitor (Qt)

APNX프로토콜을 송수신을 모니터링하는 Qt 프로그램. 

## 구조

```
APNX_qt_monitor/
├── CMakeLists.txt
└── src/
    ├── main.cpp          # QApplication 시작점
    ├── mainwindow.h/.cpp # test.txt 카드 디자인 — DEVICE CONTROL / ANALOG TAGS / PROTOCOL LOG
    ├── plcworker.h/.cpp  # /dev/myusb0 에서 TX/RX, real_test.c 참조
    └── protocol.h        # real_test.c와 동일 — STX/LEN/ID/CMD/COUNT/ADDR+DATA/CRC(Modbus 0xA001 LE)
```

## 참조 — real_test.c

- **패킷**: `STX 0x02 | LEN | ID 0x01 | CMD(READ 0/WRITE 1/ACK 5/6/NAK 15) | CMD_TYPE 0x01 | DATA_TYPE | COUNT | ADDR(LE 2B) [+DATA] | CRC(0xA001 LE)`

- **주소**: `0x100 INPUT / 0x200 OUTPUT / 0x300 DATA / 0x400 FLAG` — 이 모니터는 `0x300` DATA 영역의 4개 주소 `0x300, 0x302, 0x304, 0x306` (16-bit, count=4)를 사용

- **CRC**: `real_test.c:55 calculate_crc()` 그대로 (init `0xFFFF`, `0xA001`, LE)

- **흐름**: `plcworker.cpp:104 runRealTests()` 에서 `WRITE 4 tags → READ 4 tags` 를 정지 전까지 랜덤값으로 계속 반복, `RX`의 `DATA(7+i*2)` 4개를 `아날로그 태그` 4개 박스에 표시

## UI — test.txt 디자인 (여백 + 정보 위계 + 카드)

- **Header**: `APNX Monitor` + `● Connected/Disconnected`

- **DEVICE CONTROL**: `Device /dev/myusb0`, `Mode Hardware/Simulation`, `[Connect] [Start] [Stop]`

- **ANALOG TAGS**: `TAG1(0x0300) ~ TAG4(0x0306)` 2x2 카드 그리드, 각 카드 `여백 14px`, `값 22px bold`, 천단위 콤마

- **PROTOCOL LOG**: `TX: 02 0B ...` / `RX: ...` / `✓ READ ACK` 모노스페이스, 랜덤 4개 값 반복 로그

## 장치

- 드라이버는 `APNX/driver` 그대로 유지 — `/dev/myusb0` (fallback `/dev/myusb`) 생성
- 이 모니터는 `/dev/myusb0` 를 `open/read/write` 로 다룸 

## 빌드 / 실행

```bash
cd APNX_qt_monitor
mkdir -p build && cd build
cmake .. && make -j$(nproc)
./APNX_qt_monitor          # 또는 QT_QPA_PLATFORM=offscreen ./APNX_qt_monitor (headless 테스트)
```

- **실제 장치**: `Connect` 또는 `Start` -> `실제 장치 연결 (/dev/myusb0)` -> 랜덤 4개 값 `WRITE/READ` 반복, 박스에 RX 값 표시

- **시뮬레이션**: `Start` (하드웨어 없어도 동일 TX/RX 가짜 에코로 박스 갱신)

- **정지**: `Stop` 전까지 무한 반복

## Git으로 보내기 — 레포지토리 아직 없을 때

이 폴더 자체가 하나의 git 레포가 되도록 정리됨. 그대로 보내면 됨:

- `build/` 는 `.gitignore` 로 제외 — 소스만 커밋됨
- 펌웨어 `APNX_firmware` / 드라이버 `APNX/driver` 는 이 레포에 포함 안함 (분리)