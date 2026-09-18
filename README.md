# APNX Monitor

Qt/C++ user-space application for monitoring and testing the **APNX protocol** through the Linux USB device driver.

The monitor communicates with the APNX device through `/dev/myusb0`, generates APNX protocol packets, sends READ/WRITE requests, receives responses, and displays the received values and protocol traffic through a Qt-based GUI.

---

## Overview

APNX Monitor is the host-side application of the APNX project.

It provides a graphical interface for:

* Connecting to the APNX USB device
* Sending APNX READ/WRITE requests
* Receiving and decoding APNX responses
* Monitoring protocol TX/RX packets
* Displaying logical memory values
* Repeated READ/WRITE testing
* Running the application in hardware or simulation mode

```text
┌───────────────────────────┐
│       APNX Monitor        │
│         Qt / C++          │
│                           │
│  Device Control           │
│  Analog Tags              │
│  Protocol Log             │
└─────────────┬─────────────┘
              │
        /dev/myusb0
              │
              ▼
┌───────────────────────────┐
│     APNX USB Driver       │
│       Linux Kernel        │
└─────────────┬─────────────┘
              │
          USB Bulk
              │
              ▼
┌───────────────────────────┐
│      APNX Firmware        │
│       AVR + LUFA          │
└───────────────────────────┘
```

---

## Features

* Qt 6 / C++ GUI
* Linux device-file based communication
* APNX protocol packet generation and parsing
* READ / WRITE command support
* CRC16-Modbus calculation
* Logical address handling
* TX/RX protocol logging
* Hardware mode
* Simulation mode
* Repeated READ/WRITE test
* Real-time display of received tag values

---

## Architecture

The application is divided into three main responsibilities.

```text
┌──────────────────────────────────────┐
│              Qt GUI                 │
│                                      │
│  Device Control                     │
│  Analog Tags                        │
│  Protocol Log                       │
└─────────────────┬────────────────────┘
                  │
                  ▼
┌──────────────────────────────────────┐
│             PLC Worker               │
│                                      │
│  Device I/O                         │
│  Test Execution                     │
│  TX/RX Handling                     │
└─────────────────┬────────────────────┘
                  │
                  ▼
┌──────────────────────────────────────┐
│          APNX Protocol               │
│                                      │
│  Packet Construction                 │
│  Packet Parsing                      │
│  CRC Calculation                     │
└─────────────────┬────────────────────┘
                  │
                  ▼
             /dev/myusb0
```

### Main Components

| File                | Role                                              |
| ------------------- | ------------------------------------------------- |
| `main.cpp`          | Qt application entry point                        |
| `mainwindow.h/.cpp` | GUI and user interaction                          |
| `plcworker.h/.cpp`  | Device I/O and repeated READ/WRITE test execution |
| `protocol.h`        | APNX packet definitions and protocol handling     |
| `CMakeLists.txt`    | Build configuration                               |

---

## Device Interface

The monitor communicates with the Linux USB driver through a device file.

```text
/dev/myusb0
```

The application uses standard Linux file operations:

```text
open()
  ↓
write()
  ↓
read()
  ↓
close()
```

The Linux USB driver handles the actual USB communication with the APNX firmware.

```text
Qt Application
      │
      │ open/read/write
      ▼
/dev/myusb0
      │
      ▼
Linux USB Driver
      │
      │ USB Bulk IN/OUT
      ▼
APNX Firmware
```

---

## APNX Protocol

The monitor implements the host-side representation of the APNX packet format.

The current packet structure is:

```text
┌─────┬─────┬────┬─────┬──────────┬───────────┬───────┬─────────┬─────┐
│ STX │ LEN │ ID │ CMD │ CMD_TYPE │ DATA_TYPE │ COUNT │ PAYLOAD │ CRC │
└─────┴─────┴────┴─────┴──────────┴───────────┴───────┴─────────┴─────┘
```

### Commands

| Command   |  Value | Description          |
| --------- | -----: | -------------------- |
| READ      | `0x00` | Read logical memory  |
| WRITE     | `0x01` | Write logical memory |
| ACK_READ  | `0x05` | READ response        |
| ACK_WRITE | `0x06` | WRITE response       |
| NAK       | `0x0F` | Error response       |

### Command Type

The current implementation uses:

```text
CMD_TYPE = 0x01
```

### Data Type

The protocol supports:

| Data Type |    Size |
| --------- | ------: |
| 8-bit     |  1 byte |
| 16-bit    | 2 bytes |
| 32-bit    | 4 bytes |
| 64-bit    | 8 bytes |

---

## Logical Address Map

The monitor uses the same logical address space as the APNX firmware.

| Region | Base Address |
| ------ | -----------: |
| INPUT  |      `0x100` |
| OUTPUT |      `0x200` |
| DATA   |      `0x300` |
| FLAG   |      `0x400` |

For the current monitoring test, four 16-bit DATA values are used:

```text
0x300
0x302
0x304
0x306
```

These values are displayed as four analog tags in the GUI.

```text
┌──────────────┐  ┌──────────────┐
│ TAG 1        │  │ TAG 2        │
│ 0x0300       │  │ 0x0302       │
│ 12,345       │  │ 23,456       │
└──────────────┘  └──────────────┘

┌──────────────┐  ┌──────────────┐
│ TAG 3        │  │ TAG 4        │
│ 0x0304       │  │ 0x0306       │
│ 34,567       │  │ 45,678       │
└──────────────┘  └──────────────┘
```

---

## CRC16

The APNX protocol uses a Modbus-compatible CRC16 algorithm.

```text
Initial value : 0xFFFF
Polynomial    : 0xA001
Processing    : LSB-first
```

The monitor calculates the CRC when constructing TX packets and verifies/parses the corresponding response according to the APNX protocol.

---

## READ / WRITE Test

The monitor includes a repeated READ/WRITE test sequence.

```text
Generate values
      │
      ▼
WRITE 4 DATA tags
      │
      ▼
Send through /dev/myusb0
      │
      ▼
Receive WRITE ACK
      │
      ▼
READ 4 DATA tags
      │
      ▼
Receive READ ACK
      │
      ▼
Decode RX payload
      │
      ▼
Update GUI
      │
      └───────────────┐
                      │
                      ▼
                 Repeat
```

The current test uses:

```text
DATA_TYPE = 16-bit
COUNT     = 4

Address:
0x300
0x302
0x304
0x306
```

The test continues until the user presses `Stop`.

---

## Protocol Log

The GUI provides a protocol log for observing transmitted and received packets.

Example:

```text
TX: 02 0B 01 01 01 01 04 ...
RX: 02 ... 05 ...
✓ WRITE ACK

TX: 02 ... READ ...
RX: 02 ... 05 ... DATA ...
✓ READ ACK
```

The log is intended to make packet-level communication visible while testing the complete APNX communication path.

---

## GUI

The interface is organized into three major areas.

### Device Control

```text
DEVICE CONTROL

Device: /dev/myusb0
Mode:   Hardware / Simulation

[ Connect ] [ Start ] [ Stop ]
```

The device control area manages the communication mode and test execution.

### Analog Tags

Four logical DATA addresses are displayed as individual tag cards.

```text
TAG1   0x0300
TAG2   0x0302
TAG3   0x0304
TAG4   0x0306
```

Received values are updated from the READ response.

### Protocol Log

The protocol log displays TX/RX packets in hexadecimal form and reports successful READ/WRITE responses.

---

## Simulation Mode

The monitor supports a simulation mode for GUI and application-level testing without the physical APNX device.

```text
Simulation Mode
      │
      ▼
Generate TX packet
      │
      ▼
Simulated RX response
      │
      ▼
Decode response
      │
      ▼
Update GUI
```

This allows the GUI and monitoring flow to be tested without requiring the USB hardware.

---

## Hardware Mode

In hardware mode, the application communicates with the actual APNX device through the Linux driver.

```text
APNX Monitor
     │
     │ write()
     ▼
/dev/myusb0
     │
     ▼
APNX USB Driver
     │
     ▼
USB Bulk OUT
     │
     ▼
APNX Firmware
```

The response follows the reverse path:

```text
APNX Firmware
     │
     ▼
USB Bulk IN
     │
     ▼
APNX USB Driver
     │
     ▼
/dev/myusb0
     │
     ▼
APNX Monitor
```

---

## Build

### Requirements

* Linux
* Qt 6
* CMake
* C++ compiler
* APNX USB driver for hardware mode

### Build

```bash
git clone https://github.com/Jerry3378/APNX-monitor.git
cd APNX-monitor

mkdir -p build
cd build

cmake ..
make -j$(nproc)
```

---

## Run

### Hardware Mode

Connect the APNX device and make sure the Linux driver has created:

```text
/dev/myusb0
```

Then run:

```bash
./APNX_qt_monitor
```

Connect to the device and start the test.

The monitor repeatedly performs:

```text
WRITE 4 tags
      ↓
READ 4 tags
      ↓
Display RX values
      ↓
Repeat
```

### Simulation Mode

Simulation mode can be used without the physical APNX device.

```bash
./APNX_qt_monitor
```

Then select simulation mode from the GUI and start the test.

---

## Project Structure

```text
APNX-monitor/
│
├── CMakeLists.txt
├── README.md
│
└── src/
    ├── main.cpp
    ├── mainwindow.h
    ├── mainwindow.cpp
    ├── plcworker.h
    ├── plcworker.cpp
    └── protocol.h
```

---

## Related Components

APNX is implemented as three independent repositories.

| Component       | Repository                                                      | Role                                     |
| --------------- | --------------------------------------------------------------- | ---------------------------------------- |
| APNX Monitor    | [APNX-monitor](https://github.com/Jerry3378/APNX-monitor)       | Qt/C++ user-space monitoring and control |
| APNX USB Driver | [APNX-usb-driver](https://github.com/Jerry3378/APNX-usb-driver) | Linux kernel USB driver                  |
| APNX Firmware   | [APNX-firmware](https://github.com/Jerry3378/APNX-firmware)     | AVR USB device firmware                  |

Overall communication path:

```text
┌──────────────────────┐
│     APNX Monitor     │
│       Qt / C++       │
└──────────┬───────────┘
           │
       /dev/myusb0
           │
           ▼
┌──────────────────────┐
│   APNX USB Driver    │
│     Linux Kernel     │
└──────────┬───────────┘
           │
       USB Bulk
           │
           ▼
┌──────────────────────┐
│    APNX Firmware     │
│      AVR + LUFA      │
└──────────────────────┘
```

---

## Design Focus

The monitor is intentionally implemented as a thin user-space layer over the Linux device interface.

The main responsibilities are:

```text
User Interface
      │
      ├── Device control
      ├── Data visualization
      └── Protocol log
              │
              ▼
Protocol Handling
      │
      ├── Packet construction
      ├── Packet parsing
      └── CRC calculation
              │
              ▼
Linux Device Interface
      │
      └── /dev/myusb0
```

This separation keeps USB transport handling inside the Linux kernel driver while the Qt application focuses on protocol testing, visualization, and user interaction.

---

## License

This repository contains the APNX host-side monitoring application.

See the source files and repository history for project-specific licensing information.
