cmake_minimum_required(VERSION 3.20)

project(DroneControlSuite
    VERSION 0.1.0
    DESCRIPTION "A modular C++ mission planning and drone control framework"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(MissionControl
    src/core/main.cpp
    src/telemetry/MavlinkTelemetryReceiver.cpp
)

target_include_directories(MissionControl
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

if(WIN32)
    target_link_libraries(MissionControl PRIVATE ws2_32)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()

add_executable(SITLTelemetryTest
    tests/integration/sitl_telemetry_integration_test.cpp
    src/telemetry/MavlinkTelemetryReceiver.cpp
)

target_include_directories(SITLTelemetryTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

if(WIN32)
    target_link_libraries(SITLTelemetryTest PRIVATE ws2_32)
endif()

add_test(NAME SITLTelemetryIntegration COMMAND SITLTelemetryTest)
