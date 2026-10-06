#include "telemetry/MavlinkTelemetryReceiver.hpp"

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <thread>

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

namespace dronecontrol::telemetry {
namespace {

constexpr std::uint8_t MavlinkV1StartByte = 0xFE;
constexpr std::uint8_t MsgHeartbeat = 0;
constexpr std::uint8_t MsgAttitude = 30;
constexpr std::uint8_t MsgGlobalPositionInt = 33;
constexpr std::uint8_t MsgVfrHud = 74;

#ifdef _WIN32
constexpr auto InvalidSocket = static_cast<MavlinkTelemetryReceiver::SocketHandle>(INVALID_SOCKET);
#else
constexpr auto InvalidSocket = -1;
#endif

template <typename T>
T readLittleEndian(const std::vector<std::uint8_t>& payload, std::size_t offset) {
    if (offset + sizeof(T) > payload.size()) {
        return {};
    }

    T value{};
    std::memcpy(&value, payload.data() + offset, sizeof(T));
    return value;
}

void closeSocket(MavlinkTelemetryReceiver::SocketHandle socketHandle) {
#ifdef _WIN32
    closesocket(static_cast<SOCKET>(socketHandle));
#else
    close(socketHandle);
#endif
}

} // namespace

MavlinkTelemetryReceiver::MavlinkTelemetryReceiver(std::uint16_t listenPort)
    : listenPort_(listenPort), socket_(InvalidSocket) {}

MavlinkTelemetryReceiver::~MavlinkTelemetryReceiver() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool MavlinkTelemetryReceiver::start() {
#ifdef _WIN32
    WSADATA wsaData{};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock.\n";
        return false;
    }
#endif

    socket_ = static_cast<SocketHandle>(::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
    if (socket_ == InvalidSocket) {
        std::cerr << "Failed to create UDP socket.\n";
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(listenPort_);

#ifdef _WIN32
    const int bindResult = ::bind(static_cast<SOCKET>(socket_), reinterpret_cast<sockaddr*>(&address), sizeof(address));
#else
    const int bindResult = ::bind(socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address));
#endif
    if (bindResult < 0) {
        std::cerr << "Failed to bind UDP port " << listenPort_ << ".\n";
        closeSocket(socket_);
        socket_ = InvalidSocket;
        return false;
    }

    running_ = true;
    std::cout << "MissionControl telemetry receiver listening on UDP port " << listenPort_ << "\n";
    std::cout << "Waiting for MAVLink telemetry...\n";
    return true;
}

void MavlinkTelemetryReceiver::run() {
    if (!running_) {
        return;
    }

    std::uint8_t buffer[2048]{};
    while (running_) {
        sockaddr_in sender{};
#ifdef _WIN32
        int senderLength = sizeof(sender);
#else
        socklen_t senderLength = sizeof(sender);
#endif
#ifdef _WIN32
        const int received = recvfrom(
            static_cast<SOCKET>(socket_),
            reinterpret_cast<char*>(buffer),
            static_cast<int>(sizeof(buffer)),
            0,
            reinterpret_cast<sockaddr*>(&sender),
            &senderLength);
#else
        const int received = recvfrom(
            socket_,
            reinterpret_cast<char*>(buffer),
            static_cast<int>(sizeof(buffer)),
            0,
            reinterpret_cast<sockaddr*>(&sender),
            &senderLength);
#endif

        if (received <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        processBytes(buffer, received);
    }
}

void MavlinkTelemetryReceiver::stop() {
    running_ = false;
    if (socket_ != InvalidSocket) {
        closeSocket(socket_);
        socket_ = InvalidSocket;
    }
}

void MavlinkTelemetryReceiver::processBytes(const std::uint8_t* data, int size) {
    int index = 0;
    while (index < size) {
        if (data[index] != MavlinkV1StartByte) {
            ++index;
            continue;
        }

        if (index + 8 > size) {
            break;
        }

        const std::uint8_t payloadLength = data[index + 1];
        const int packetLength = 6 + payloadLength + 2;
        if (index + packetLength > size) {
            break;
        }

        const std::uint8_t messageId = data[index + 5];
        std::vector<std::uint8_t> payload(data + index + 6, data + index + 6 + payloadLength);
        processPacket(messageId, payload);

        index += packetLength;
    }
}

void MavlinkTelemetryReceiver::processPacket(std::uint8_t messageId, const std::vector<std::uint8_t>& payload) {
    bool shouldPrint = false;

    switch (messageId) {
    case MsgHeartbeat:
        if (!state_.heartbeatReceived) {
            std::cout << "HEARTBEAT received.\n";
        }
        state_.heartbeatReceived = true;
        shouldPrint = true;
        break;

    case MsgGlobalPositionInt: {
        const auto lat = readLittleEndian<std::int32_t>(payload, 4);
        const auto lon = readLittleEndian<std::int32_t>(payload, 8);
        const auto relativeAlt = readLittleEndian<std::int32_t>(payload, 16);

        state_.latitudeDeg = static_cast<double>(lat) / 1e7;
        state_.longitudeDeg = static_cast<double>(lon) / 1e7;
        state_.altitudeM = static_cast<double>(relativeAlt) / 1000.0;
        state_.positionReceived = true;
        shouldPrint = true;
        break;
    }

    case MsgAttitude:
        state_.rollRad = readLittleEndian<float>(payload, 4);
        state_.pitchRad = readLittleEndian<float>(payload, 8);
        state_.yawRad = readLittleEndian<float>(payload, 12);
        state_.attitudeReceived = true;
        shouldPrint = true;
        break;

    case MsgVfrHud:
        state_.groundSpeed = readLittleEndian<float>(payload, 4);
        state_.headingDeg = readLittleEndian<std::int16_t>(payload, 8);
        state_.climbRate = readLittleEndian<float>(payload, 16);
        state_.hudReceived = true;
        shouldPrint = true;
        break;

    default:
        break;
    }

    if (shouldPrint && state_.positionReceived) {
        printState();
    }
}

void MavlinkTelemetryReceiver::printState() const {
    std::cout << std::fixed << std::setprecision(7)
              << "Telemetry | lat=" << state_.latitudeDeg
              << " lon=" << state_.longitudeDeg
              << std::setprecision(1) << " alt=" << state_.altitudeM << "m"
              << " heading=" << state_.headingDeg
              << std::setprecision(3) << " roll=" << state_.rollRad
              << " pitch=" << state_.pitchRad
              << " yaw=" << state_.yawRad << '\n';
}

} // namespace dronecontrol::telemetry
