// plcworker.cpp
// APNX와 동일한 프레임 규칙으로 TX/RX 모니터링

#include "plcworker.h"
#include "protocol.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <string>

// 장치 경로
static const char *DEVICE_PATH = "/dev/myusb0";
static const char *DEVICE_PATH_FALLBACK = "/dev/myusb";

// calculate_crc 그대로 (Modbus 0xA001, LE)
static inline uint16_t calc_crc(const uint8_t *buf, size_t len) {
    return calculate_crc(buf, len);
}

static QString toHex(const uint8_t *buf, size_t len) {
    QString s;
    for (size_t i = 0; i < len; i++) s += QString("%1 ").arg(buf[i], 2, 16, QLatin1Char('0'));
    return s.trimmed().toUpper();
}

// APNX프로토콜 방식을 그대로 따름: STX/LEN/ID/CMD/CMD_TYPE/DATA_TYPE/COUNT + ADDR(LE) [+DATA] + CRC(LE)
static int buildWritePacket(uint8_t *pkt, uint16_t addr, uint8_t data_type, const uint8_t *data, uint8_t count, uint8_t cmd = CMD_WRITE) {
    int off = 0;
    pkt[off++] = 0x02;
    pkt[off++] = 0x00; // LEN placeholder
    pkt[off++] = 0x01; // ID
    pkt[off++] = cmd;
    pkt[off++] = 0x01; // CMD_TYPE
    pkt[off++] = data_type;
    pkt[off++] = count;
    uint8_t ds = (1u << data_type);
    for (int i = 0; i < count; i++) {
        pkt[off++] = addr & 0xFF;
        pkt[off++] = (addr >> 8) & 0xFF;
        memcpy(pkt + off, data + i*ds, ds);
        off += ds;
        addr += ds;
    }
    pkt[1] = off + 2;
    uint16_t crc = calc_crc(pkt, off);
    memcpy(pkt + off, &crc, 2);
    off += 2;
    return off;
}

static int buildReadPacket(uint8_t *pkt, uint16_t addr, uint8_t data_type, uint8_t count) {
    int off = 0;
    pkt[off++] = 0x02;
    pkt[off++] = 0x00;
    pkt[off++] = 0x01;
    pkt[off++] = CMD_READ;
    pkt[off++] = 0x01;
    pkt[off++] = data_type;
    pkt[off++] = count;
    uint8_t ds = (1u << data_type);
    for (int i = 0; i < count; i++) {
        pkt[off++] = addr & 0xFF;
        pkt[off++] = (addr >> 8) & 0xFF;
        addr += ds;
    }
    pkt[1] = off + 2;
    uint16_t crc = calc_crc(pkt, off);
    memcpy(pkt + off, &crc, 2);
    off += 2;
    return off;
}

PlcWorker::PlcWorker(QObject *parent) : QThread(parent) {}
void PlcWorker::setSimulation(bool on) { m_simulation = on; }
void PlcWorker::requestStop() { m_stopRequested = true; }

void PlcWorker::run() {
    m_stopRequested = false;
    if (m_simulation) {
        emit connectionStatusChanged("시뮬레이션 (real_test 기반 TX/RX)");
        runSimulatedTests();
        return;
    }
    emit logMessage(QString("장치 열기: %1").arg(DEVICE_PATH));
    int fd = ::open(DEVICE_PATH, O_RDWR);
    if (fd < 0) fd = ::open(DEVICE_PATH_FALLBACK, O_RDWR);
    if (fd < 0) {
        emit connectionStatusChanged("연결 실패");
        emit logMessage(QString("open 실패: %1").arg(strerror(errno)));
        emit logMessage("힌트: sudo chmod 666 /dev/myusb0");
        return;
    }
    emit connectionStatusChanged("연결됨 (/dev/myusb0)");
    emit logMessage("real_test 기반 패킷 테스트 시작");
    runRealTests(fd);
    ::close(fd);
    emit connectionStatusChanged("연결 종료");
}

// real_test.c 의 main() TEST 흐름을 그대로 재현 — TX/RX 로그 위주
void PlcWorker::runRealTests(int fd) {
    uint8_t resp[256];

    auto doWrite = [&](uint16_t addr, uint8_t dtype, const uint8_t* d, uint8_t cnt) {
        uint8_t pkt[256]; int len = buildWritePacket(pkt, addr, dtype, d, cnt);
        emit logMessage(QString("TX: %1").arg(toHex(pkt, len)));
        if (::write(fd, pkt, len) < 0) { emit logMessage(QString("write fail: %1").arg(strerror(errno))); return; }
        int n = ::read(fd, resp, sizeof(resp));
        if (n > 0) {
            emit logMessage(QString("RX: %1").arg(toHex(resp, n)));
            if (resp[3] == CMD_ACK_WRITE) emit logMessage("✓ WRITE ACK");
            else if (resp[3] == CMD_NAK) emit logMessage("✗ WRITE NAK");
        }
    };

    auto doRead = [&](uint16_t addr, uint8_t dtype, uint8_t cnt) {
        uint8_t pkt[256]; int len = buildReadPacket(pkt, addr, dtype, cnt);
        emit logMessage(QString("TX: %1").arg(toHex(pkt, len)));
        if (::write(fd, pkt, len) < 0) { emit logMessage(QString("write fail: %1").arg(strerror(errno))); return; }
        int n = ::read(fd, resp, sizeof(resp));
        if (n > 0) {
            emit logMessage(QString("RX: %1").arg(toHex(resp, n)));
            if (resp[3] == CMD_ACK_READ) {
                emit logMessage("✓ READ ACK");
                // 데이터 추출 예시: offset 7부터
                size_t ds = (1u << dtype);
                QVector<double> vals;
                for (int i = 0; i < cnt; i++) {
                    uint16_t v = 0; // 16비트 예시
                    if (ds >= 2) v = resp[7 + i*ds] | (resp[7 + i*ds +1] << 8);
                    else v = resp[7 + i*ds];
                    vals.push_back(v);
                }
                emit sensorDataUpdated(vals, resp[3], resp[2]);
            } else if (resp[3] == CMD_NAK) emit logMessage("✗ READ NAK");
        }
    };

    // 계속 반복 — 4개 태그(4개 메모리 주소)에 랜덤값을 계속 보내고 RX로 확인
    //  TAG1=0x300, TAG2=0x302, TAG3=0x304, TAG4=0x306 (16-bit, count=4)
    //  펌웨어는 에코이므로 보낸 4개 값이 RX로 그대로 돌아와 4개 박스에 찍힘
    srand(time(nullptr) ^ getpid());
    int iter = 0;
    while (!m_stopRequested) {
        iter++;
        uint16_t rnd4[4];
        for (int i=0;i<4;i++) rnd4[i] = rand() & 0xFFFF;

        emit logMessage(QString("========== iter %1: WRITE 4 tags 0x300~0x306 (rand %2 %3 %4 %5) ==========")
            .arg(iter).arg(rnd4[0]).arg(rnd4[1]).arg(rnd4[2]).arg(rnd4[3]));
        doWrite(0x300, DATA_TYPE_16, (uint8_t*)rnd4, 4); // 4개 주소에 한 번에 WRITE
        if (m_stopRequested) break; usleep(250000);
        emit logMessage(QString("========== iter %1: READ 4 tags 0x300~0x306 ==========").arg(iter));
        doRead(0x300, DATA_TYPE_16, 4); // 4개 주소 한 번에 READ → RX 4개 값이 4 박스에 표시
        if (m_stopRequested) break; usleep(400000);
    }
}

// 하드웨어 없이도 TX/RX 에코 — 4개 태그 랜덤값으로 반복
void PlcWorker::runSimulatedTests() {
    srand(time(nullptr) ^ getpid());
    uint8_t pkt[256];
    int iter=0;
    while (!m_stopRequested) {
        iter++;
        uint16_t rnd4[4];
        for (int i=0;i<4;i++) rnd4[i]=rand()&0xFFFF;
        int len = buildWritePacket(pkt, 0x300, DATA_TYPE_16, (uint8_t*)rnd4, 4);
        emit logMessage(QString("TX: %1  (WRITE 4 tags iter %2)").arg(toHex(pkt,len)).arg(iter));
        uint8_t rx[256]; int rlen = buildWritePacket(rx, 0x300, DATA_TYPE_16, (uint8_t*)rnd4, 4, CMD_ACK_WRITE);
        emit logMessage(QString("RX: %1  (sim WRITE ACK iter %2)").arg(toHex(rx,rlen)).arg(iter));
        usleep(250000);
        if (m_stopRequested) break;
        len = buildReadPacket(pkt, 0x300, DATA_TYPE_16, 4);
        emit logMessage(QString("TX: %1  (READ 4 tags iter %2)").arg(toHex(pkt,len)).arg(iter));
        rlen = buildWritePacket(rx, 0x300, DATA_TYPE_16, (uint8_t*)rnd4, 4, CMD_ACK_READ);
        rx[3]=CMD_ACK_READ;
        emit logMessage(QString("RX: %1  (sim READ ACK iter %2, values %3 %4 %5 %6)")
            .arg(toHex(rx,rlen)).arg(iter).arg(rnd4[0]).arg(rnd4[1]).arg(rnd4[2]).arg(rnd4[3]));
        QVector<double> vals; for(int i=0;i<4;i++) vals.push_back(rnd4[i]);
        emit sensorDataUpdated(vals, CMD_ACK_READ, 0x01);
        usleep(400000);
    }
}
