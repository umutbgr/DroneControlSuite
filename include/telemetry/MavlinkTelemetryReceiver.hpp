#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dronecontrol::telemetry {

struct TelemetryState {
    bool heartbeatReceived{false};
    bool positionReceived{false};
    bool attitudeReceived{false};
    bool hudReceived{false};

    double latitudeDeg{0.0};
    double longitudeDeg{0.0};
    double altitudeM{0.0};
    double rollRad{0.0};
    double pitchRad{0.0};
    double yawRad{0.0};
    double groundSpeed{0.0};
    double climbRate{0.0};
    int headingDeg{0};
};

class MavlinkTelemetryReceiver {
public:
#ifdef _WIN32
    using SocketHandle = std::uintptr_t;
#else
    using SocketHandle = int;
#endif

    explicit MavlinkTelemetryReceiver(std::uint16_t listenPort);
    ~MavlinkTelemetryReceiver();

    MavlinkTelemetryReceiver(const MavlinkTelemetryReceiver&) = delete;
    MavlinkTelemetryReceiver& operator=(const MavlinkTelemetryReceiver&) = delete;

    bool start();
    void run();
    void stop();

    const TelemetryState& getState() const noexcept { return state_; }

private:
    void processBytes(const std::uint8_t* data, int size);
    void processPacket(std::uint8_t messageId, const std::vector<std::uint8_t>& payload);
    void printState() const;

    std::uint16_t listenPort_;
    bool running_{false};
    TelemetryState state_{};

    SocketHandle socket_{};
};

} // namespace dronecontrol::telemetry
