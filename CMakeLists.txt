cmake_minimum_required(VERSION 3.16)

# APNX_qt_monitor  (원본: miniplc_qt_gui)
# ============================================================================
# 리네이밍: miniplc_qt_gui → APNX_qt_monitor
# 원본 경로: /home/jaemopark/miniplc_project/miniplc_qt_gui/CMakeLists.txt
# 빌드:
#   mkdir -p build && cd build && cmake .. && make -j$(nproc) && ./APNX_qt_monitor
# ============================================================================
project(APNX_qt_monitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 자동 처리 (MOC/RCC/UIC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt5 Widgets (Ubuntu 22.04 기본). Qt6 사용 시 Qt6로 교체
find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/plcworker.cpp
    src/plcworker.h
    src/protocol.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets)
