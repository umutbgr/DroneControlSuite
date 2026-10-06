#include "telemetry/MavlinkTelemetryReceiver.hpp"
#include "ui/TelemetryDisplay.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <thread>

namespace {
    std::atomic<bool> g_running{true};

    void onSignal(int) { g_running = false; }
}

int main() {
    constexpr std::uint16_t telemetryPort = 14600; // local_mavlink_sim.py output port

    std::signal(SIGINT,  onSignal);
    std::signal(SIGTERM, onSignal);

    dronecontrol::ui::enableAnsiConsole();

    dronecontrol::telemetry::MavlinkTelemetryReceiver receiver(telemetryPort);
    if (!receiver.start()) {
        std::cerr << "Failed to start telemetry receiver on port " << telemetryPort << '\n';
        return 1;
    }

    // Receiver blocks in recvfrom — run it on a dedicated thread.
    std::thread recvThread([&] { receiver.run(); });

    dronecontrol::ui::TelemetryDisplay display(receiver);

    while (g_running) {
        display.render();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    receiver.stop();
    recvThread.join();
    return 0;
}
