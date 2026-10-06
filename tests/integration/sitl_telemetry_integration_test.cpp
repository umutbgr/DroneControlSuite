// SITL integration tests for MavlinkTelemetryReceiver.
// Each test spins up a real UDP receiver, injects raw MAVLink v1 packets
// from a sender socket, then stops the receiver and verifies the parsed state.
// No external test framework — the binary returns 0 on success, 1 on failure.

#include "telemetry/MavlinkTelemetryReceiver.hpp"

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

// ---------------------------------------------------------------------------
// MAVLink v1 message IDs (subset used by MavlinkTelemetryReceiver)
// ---------------------------------------------------------------------------
constexpr std::uint8_t MsgHeartbeat        = 0;
constexpr std::uint8_t MsgAttitude         = 30;
constexpr std::uint8_t MsgGlobalPositionInt = 33;
constexpr std::uint8_t MsgVfrHud           = 74;

// Dedicated test port — must not conflict with the live SITL ports (14550/14551)
constexpr std::uint16_t TestPort = 14661;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Write value T at byte offset in buf (little-endian), growing buf if needed.
template <typename T>
static void writeLE(std::vector<std::uint8_t>& buf, std::size_t offset, T value) {
    if (buf.size() < offset + sizeof(T)) {
        buf.resize(offset + sizeof(T), 0);
    }
    std::memcpy(buf.data() + offset, &value, sizeof(T));
}

// Build a MAVLink v1 packet.  The receiver does not check the CRC, so we
// leave it as zeroes to keep test code simple.
static std::vector<std::uint8_t> buildPacket(std::uint8_t msgId,
                                              const std::vector<std::uint8_t>& payload) {
    std::vector<std::uint8_t> pkt;
    pkt.reserve(8 + payload.size());
    pkt.push_back(0xFE);                                      // STX
    pkt.push_back(static_cast<std::uint8_t>(payload.size())); // LEN
    pkt.push_back(0x00);                                      // SEQ
    pkt.push_back(0x01);                                      // SYSID
    pkt.push_back(0x01);                                      // COMPID
    pkt.push_back(msgId);                                     // MSGID
    pkt.insert(pkt.end(), payload.begin(), payload.end());
    pkt.push_back(0x00); // CRC_LO (not validated)
    pkt.push_back(0x00); // CRC_HI (not validated)
    return pkt;
}

// Send a single UDP datagram to 127.0.0.1:port.  Returns true on success.
static bool sendUdp(std::uint16_t port, const std::vector<std::uint8_t>& data) {
#ifdef _WIN32
    SOCKET s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) { return false; }
#else
    int s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0) { return false; }
#endif

    sockaddr_in dest{};
    dest.sin_family      = AF_INET;
    dest.sin_port        = htons(port);
    dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

#ifdef _WIN32
    const int sent = ::sendto(s,
        reinterpret_cast<const char*>(data.data()),
        static_cast<int>(data.size()), 0,
        reinterpret_cast<const sockaddr*>(&dest), sizeof(dest));
    ::closesocket(s);
    return sent == static_cast<int>(data.size());
#else
    const ssize_t sent = ::sendto(s,
        data.data(), data.size(), 0,
        reinterpret_cast<const sockaddr*>(&dest), sizeof(dest));
    ::close(s);
    return sent == static_cast<ssize_t>(data.size());
#endif
}

// ---------------------------------------------------------------------------
// Test runner
// ---------------------------------------------------------------------------

static int g_pass = 0;
static int g_fail = 0;

static void check(bool cond, const char* name) {
    if (cond) {
        std::cout << "  [PASS] " << name << '\n';
        ++g_pass;
    } else {
        std::cout << "  [FAIL] " << name << '\n';
        ++g_fail;
    }
}

// Start a receiver on TestPort, send each datagram in `packets` (with a short
// inter-packet delay), wait for processing, stop the receiver and return the
// final TelemetryState.
static dronecontrol::telemetry::TelemetryState runReceiver(
    std::initializer_list<std::vector<std::uint8_t>> packets) {

    dronecontrol::telemetry::MavlinkTelemetryReceiver receiver(TestPort);
    if (!receiver.start()) {
        std::cerr << "  ERROR: failed to start receiver on port " << TestPort << '\n';
        return {};
    }

    std::thread recvThread([&] { receiver.run(); });

    // Give the receiver thread time to enter recvfrom.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    for (const auto& pkt : packets) {
        sendUdp(TestPort, pkt);
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    // Allow the last packet to be processed before we stop.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    receiver.stop();
    recvThread.join();

    return receiver.getState();
}

// ---------------------------------------------------------------------------
// Individual tests
// ---------------------------------------------------------------------------

static void testHeartbeat() {
    std::cout << "\n[Test] HEARTBEAT detection\n";

    std::vector<std::uint8_t> payload(9, 0); // 9-byte HEARTBEAT payload
    auto state = runReceiver({buildPacket(MsgHeartbeat, payload)});

    check(state.heartbeatReceived,  "heartbeat flag set after HEARTBEAT");
    check(!state.positionReceived,  "position flag not set without GLOBAL_POSITION_INT");
    check(!state.attitudeReceived,  "attitude flag not set without ATTITUDE");
}

static void testGlobalPositionInt() {
    std::cout << "\n[Test] GLOBAL_POSITION_INT parsing\n";

    // Field layout (MAVLink v1, offsets relative to payload start):
    //   0  time_boot_ms  uint32
    //   4  lat           int32   (1e-7 deg)
    //   8  lon           int32   (1e-7 deg)
    //  12  alt           int32   (mm, absolute)
    //  16  relative_alt  int32   (mm, above home)
    //  20  vx            int16
    //  22  vy            int16
    //  24  vz            int16
    //  26  hdg           uint16
    std::vector<std::uint8_t> payload(28, 0);

    const double wantLat = 37.8749;
    const double wantLon = 32.4922;
    const double wantAlt = 50.0; // metres

    writeLE(payload,  4, static_cast<std::int32_t>(wantLat * 1e7));
    writeLE(payload,  8, static_cast<std::int32_t>(wantLon * 1e7));
    writeLE(payload, 16, static_cast<std::int32_t>(wantAlt * 1000.0));

    auto state = runReceiver({buildPacket(MsgGlobalPositionInt, payload)});

    check(state.positionReceived,                           "position flag set");
    check(std::abs(state.latitudeDeg  - wantLat) < 1e-5,   "latitude parsed correctly");
    check(std::abs(state.longitudeDeg - wantLon) < 1e-5,   "longitude parsed correctly");
    check(std::abs(state.altitudeM    - wantAlt) < 0.01,   "altitude parsed correctly");
}

static void testAttitude() {
    std::cout << "\n[Test] ATTITUDE message parsing\n";

    // Field layout:
    //   0  time_boot_ms  uint32
    //   4  roll          float (rad)
    //   8  pitch         float (rad)
    //  12  yaw           float (rad)
    //  16  rollspeed     float
    //  20  pitchspeed    float
    //  24  yawspeed      float
    std::vector<std::uint8_t> attPayload(28, 0);
    const float wantRoll  = 0.10f;
    const float wantPitch = 0.20f;
    const float wantYaw   = 1.57f;

    writeLE(attPayload,  4, wantRoll);
    writeLE(attPayload,  8, wantPitch);
    writeLE(attPayload, 12, wantYaw);

    // GLOBAL_POSITION_INT must arrive first so printState() fires and state is
    // persisted; the receiver only prints when positionReceived is true.
    std::vector<std::uint8_t> posPayload(28, 0);
    writeLE(posPayload,  4, static_cast<std::int32_t>(37.0 * 1e7));
    writeLE(posPayload,  8, static_cast<std::int32_t>(32.0 * 1e7));
    writeLE(posPayload, 16, static_cast<std::int32_t>(10 * 1000));

    auto state = runReceiver({
        buildPacket(MsgGlobalPositionInt, posPayload),
        buildPacket(MsgAttitude,          attPayload)
    });

    check(state.attitudeReceived,                              "attitude flag set");
    check(std::abs(state.rollRad  - wantRoll)  < 1e-5f,       "roll parsed correctly");
    check(std::abs(state.pitchRad - wantPitch) < 1e-5f,       "pitch parsed correctly");
    check(std::abs(state.yawRad   - wantYaw)   < 1e-5f,       "yaw parsed correctly");
}

static void testVfrHud() {
    std::cout << "\n[Test] VFR_HUD message parsing\n";

    // Field layout (MAVLink common.xml wire order):
    //   0  airspeed    float (m/s)
    //   4  groundspeed float (m/s)
    //   8  alt         float (m)
    //  12  climb       float (m/s)
    //  16  heading     int16 (deg, 0-359)
    //  18  throttle    uint16
    std::vector<std::uint8_t> hudPayload(20, 0);
    const float   wantGS      = 5.5f;
    const std::int16_t wantHdg = 180;
    const float   wantClimb   = 1.2f;

    writeLE(hudPayload,  4, wantGS);
    writeLE(hudPayload, 12, wantClimb);
    writeLE(hudPayload, 16, wantHdg);

    std::vector<std::uint8_t> posPayload(28, 0);
    writeLE(posPayload, 16, static_cast<std::int32_t>(5 * 1000));

    auto state = runReceiver({
        buildPacket(MsgGlobalPositionInt, posPayload),
        buildPacket(MsgVfrHud,            hudPayload)
    });

    check(state.hudReceived,                                  "HUD flag set");
    check(std::abs(state.groundSpeed - wantGS)    < 1e-4f,   "ground speed parsed correctly");
    check(state.headingDeg == static_cast<int>(wantHdg),      "heading parsed correctly");
    check(std::abs(state.climbRate   - wantClimb) < 1e-4f,   "climb rate parsed correctly");
}

static void testMultiplePacketsInOneDatagram() {
    std::cout << "\n[Test] Multiple MAVLink packets concatenated in one UDP datagram\n";

    std::vector<std::uint8_t> hbPayload(9, 0);

    std::vector<std::uint8_t> posPayload(28, 0);
    const double wantLat = 39.9;
    const double wantLon = 32.8;
    writeLE(posPayload,  4, static_cast<std::int32_t>(wantLat * 1e7));
    writeLE(posPayload,  8, static_cast<std::int32_t>(wantLon * 1e7));
    writeLE(posPayload, 16, static_cast<std::int32_t>(20 * 1000));

    auto hbPkt  = buildPacket(MsgHeartbeat,         hbPayload);
    auto posPkt = buildPacket(MsgGlobalPositionInt, posPayload);

    // Concatenate into a single UDP datagram
    std::vector<std::uint8_t> combined;
    combined.insert(combined.end(), hbPkt.begin(),  hbPkt.end());
    combined.insert(combined.end(), posPkt.begin(), posPkt.end());

    auto state = runReceiver({combined});

    check(state.heartbeatReceived,                            "heartbeat from combined datagram");
    check(state.positionReceived,                             "position from combined datagram");
    check(std::abs(state.latitudeDeg  - wantLat) < 1e-5,     "latitude correct from combined datagram");
    check(std::abs(state.longitudeDeg - wantLon) < 1e-5,     "longitude correct from combined datagram");
}

static void testUnknownMessageIdIgnored() {
    std::cout << "\n[Test] Unknown message ID is silently ignored\n";

    // Send a packet with a message ID not handled by the receiver
    std::vector<std::uint8_t> payload(10, 0xAB);
    auto state = runReceiver({buildPacket(0xFF, payload)});

    check(!state.heartbeatReceived, "heartbeat flag not set");
    check(!state.positionReceived,  "position flag not set");
    check(!state.attitudeReceived,  "attitude flag not set");
    check(!state.hudReceived,       "HUD flag not set");
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main() {
    std::cout << "=== DroneControlSuite SITL Integration Tests ===\n";
    std::cout << "Receiver test port: " << TestPort << '\n';

    testHeartbeat();
    testGlobalPositionInt();
    testAttitude();
    testVfrHud();
    testMultiplePacketsInOneDatagram();
    testUnknownMessageIdIgnored();

    std::cout << "\n=== Results: " << g_pass << " passed, "
              << g_fail << " failed ===\n";

    return g_fail > 0 ? 1 : 0;
}
