// protocol.h
// real_test.c 와 동일한 규칙 — 펌웨어(MiniPLC.c) 참조
//  형식: STX 0x02 | LEN | ID 0x01 | CMD | CMD_TYPE 0x01 | DATA_TYPE | COUNT | ADDR(LE 2B) [+DATA] | CRC(Modbus 0xA001, LE)
//  주소: 0x100 INPUT, 0x200 OUTPUT, 0x300 DATA, 0x400 FLAG
//  데이터 크기: 1<<DATA_TYPE (0:1B, 1:2B, 2:4B, 3:8B)

#pragma once
#include <cstdint>
#include <vector>

enum Command : uint8_t {
    CMD_READ = 0,
    CMD_WRITE = 1,
    CMD_ACK_READ = 5,
    CMD_ACK_WRITE = 6,
    CMD_NAK = 15
};

enum DataType : uint8_t {
    DATA_TYPE_8 = 0,
    DATA_TYPE_16 = 1,
    DATA_TYPE_32 = 2,
    DATA_TYPE_64 = 3
};

enum LogicalAddr : uint16_t {
    ADDR_INPUT = 0x100,
    ADDR_OUTPUT = 0x200,
    ADDR_DATA = 0x300,
    ADDR_FLAG = 0x400
};

// real_test.c: calculate_crc — Modbus 0xA001, init 0xFFFF, LE
inline uint16_t calculate_crc(const uint8_t *buf, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}

// 파싱 결과 — real_test.c 처럼 주소+데이터 함께 보관
struct PlcPacket {
    uint8_t stx = 0;
    uint8_t len = 0;
    uint8_t id = 0;
    uint8_t cmd = 0;
    uint8_t cmdType = 0;
    uint8_t dataType = 0;
    uint8_t count = 0;
    std::vector<uint16_t> addrs;   // 각 entry의 주소
    std::vector<uint8_t> data;     // raw 데이터 (LE)
    uint16_t crc = 0;
};

// 간단 파싱: real_test 처럼 헤더+주소+데이터 분리, CRC는 LE로 읽음
inline bool parsePlcPacket(const uint8_t *buf, size_t size, PlcPacket &out) {
    if (size < 9) return false;
    out.stx = buf[0];
    out.len = buf[1];
    out.id = buf[2];
    out.cmd = buf[3];
    out.cmdType = buf[4];
    out.dataType = buf[5];
    out.count = buf[6];
    if (out.len != size) return false;
    out.crc = buf[size-2] | (buf[size-1] << 8); // LE
    size_t data_size = (1u << out.dataType);
    size_t pos = 7;
    out.addrs.clear();
    out.data.clear();
    for (uint8_t i = 0; i < out.count; i++) {
        if (pos + 2 > size - 2) break;
        uint16_t addr = buf[pos] | (buf[pos+1] << 8);
        out.addrs.push_back(addr);
        pos += 2;
        // WRITE 계열은 뒤에 데이터가 있음, READ 요청은 주소만
        if (out.cmd == CMD_WRITE || out.cmd == CMD_ACK_READ || out.cmd == CMD_ACK_WRITE) {
            if (pos + data_size > size - 2) break;
            for (size_t k = 0; k < data_size; k++) out.data.push_back(buf[pos+k]);
            pos += data_size;
        }
    }
    return true;
}
