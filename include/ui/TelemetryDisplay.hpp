#pragma once

#include "telemetry/MavlinkTelemetryReceiver.hpp"

#include <cstdio>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <algorithm>

namespace dronecontrol::ui {

// Enable ANSI escape sequences on Windows 10+.
inline void enableAnsiConsole() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
}

// Minimal ANSI wrapper — only what we actually use.
namespace ansi {
    constexpr const char* reset   = "\033[0m";
    constexpr const char* bold    = "\033[1m";
    constexpr const char* red     = "\033[31m";
    constexpr const char* green   = "\033[32m";
    constexpr const char* yellow  = "\033[33m";
    constexpr const char* cyan    = "\033[36m";
    constexpr const char* white   = "\033[37m";
    constexpr const char* clearScreen = "\033[2J\033[H";
}

// Console telemetry dashboard for MavlinkTelemetryReceiver.
// Call render() in a loop to refresh the display in place.
class TelemetryDisplay {
public:
    static constexpr int Width = 48;

    explicit TelemetryDisplay(const telemetry::MavlinkTelemetryReceiver& receiver)
        : receiver_(receiver) {}

    void render() const {
        const auto& s = receiver_.getState();
        std::ostringstream out;

        out << ansi::clearScreen;
        header(out);
        statusRow(out, s);
        separator(out);
        positionSection(out, s);
        separator(out);
        attitudeSection(out, s);
        separator(out);
        navigationSection(out, s);
        footer(out);

        std::fputs(out.str().c_str(), stdout);
        std::fflush(stdout);
    }

private:
    const telemetry::MavlinkTelemetryReceiver& receiver_;

    static void separator(std::ostringstream& o) {
        o << ansi::cyan << '+' << std::string(Width - 2, '-') << '+' << ansi::reset << '\n';
    }

    static void header(std::ostringstream& o) {
        const std::string title = " DroneControl Suite - MissionControl ";
        const int pad = (Width - 2 - static_cast<int>(title.size())) / 2;
        o << ansi::cyan << '+' << std::string(Width - 2, '-') << '+' << ansi::reset << '\n';
        o << ansi::cyan << '|' << ansi::reset
          << ansi::bold << std::string(pad, ' ') << title << std::string(pad, ' ') << ansi::reset
          << ansi::cyan << '|' << ansi::reset << '\n';
        separator(o);
    }

    static void statusRow(std::ostringstream& o, const telemetry::TelemetryState& s) {
        const bool connected = s.heartbeatReceived;
        const char* color = connected ? ansi::green : ansi::red;
        const char* label = connected ? "CONNECTED" : "WAITING...";
        row(o, " Status", color, label);
    }

    static void positionSection(std::ostringstream& o, const telemetry::TelemetryState& s) {
        sectionHeader(o, "POSITION");
        if (s.positionReceived) {
            rowDouble(o, " Latitude ",  s.latitudeDeg,  7, "deg");
            rowDouble(o, " Longitude", s.longitudeDeg, 7, "deg");
            rowDouble(o, " Altitude ",  s.altitudeM,    1, "m  ");
        } else {
            rowEmpty(o, " No position data");
        }
    }

    static void attitudeSection(std::ostringstream& o, const telemetry::TelemetryState& s) {
        sectionHeader(o, "ATTITUDE");
        if (s.attitudeReceived) {
            rowDouble(o, " Roll  ", s.rollRad,  3, "rad");
            rowDouble(o, " Pitch ", s.pitchRad, 3, "rad");
            rowDouble(o, " Yaw   ", s.yawRad,   3, "rad");
        } else {
            rowEmpty(o, " No attitude data");
        }
    }

    static void navigationSection(std::ostringstream& o, const telemetry::TelemetryState& s) {
        sectionHeader(o, "NAVIGATION");
        if (s.hudReceived) {
            rowInt(o,    " Heading     ", s.headingDeg, "deg");
            rowDouble(o, " Ground Speed", s.groundSpeed, 1, "m/s");
            rowDouble(o, " Climb Rate  ", s.climbRate,   1, "m/s");
        } else {
            rowEmpty(o, " No HUD data");
        }
    }

    static void footer(std::ostringstream& o) {
        o << ansi::cyan << '+' << std::string(Width - 2, '-') << '+' << ansi::reset << '\n';
        o << ansi::white << " Press Ctrl+C to exit" << ansi::reset << '\n';
    }

    static void sectionHeader(std::ostringstream& o, const std::string& title) {
        std::string line = " " + title;
        line.resize(Width - 2, ' ');
        o << ansi::cyan << '|' << ansi::reset
          << ansi::bold << ansi::yellow << line << ansi::reset
          << ansi::cyan << '|' << ansi::reset << '\n';
    }

    static void row(std::ostringstream& o,
                    const std::string& label,
                    const char* valueColor,
                    const std::string& value) {
        std::ostringstream inner;
        inner << " " << std::left << std::setw(12) << label << ": "
              << valueColor << std::left << std::setw(20) << value << ansi::reset;
        const std::string content = inner.str();

        // Count visible chars (strip escape sequences for padding)
        const std::string visible = label + ":  " + value;
        const int visibleLen = 1 + 12 + 2 + static_cast<int>(value.size());
        const int padding = std::max(0, Width - 2 - visibleLen - 1);

        o << ansi::cyan << '|' << ansi::reset
          << " " << std::left << std::setw(13) << label << ": "
          << valueColor << value << ansi::reset
          << std::string(padding + (20 - static_cast<int>(value.size())), ' ')
          << ansi::cyan << '|' << ansi::reset << '\n';
    }

    static void rowDouble(std::ostringstream& o,
                          const std::string& label,
                          double val, int prec,
                          const std::string& unit) {
        std::ostringstream vs;
        vs << std::fixed << std::setprecision(prec) << val << ' ' << unit;
        row(o, label, ansi::white, vs.str());
    }

    static void rowInt(std::ostringstream& o,
                       const std::string& label,
                       int val,
                       const std::string& unit) {
        std::ostringstream vs;
        vs << val << ' ' << unit;
        row(o, label, ansi::white, vs.str());
    }

    static void rowEmpty(std::ostringstream& o, const std::string& msg) {
        std::string line = msg;
        line.resize(Width - 2, ' ');
        o << ansi::cyan << '|' << ansi::reset
          << ansi::white << line << ansi::reset
          << ansi::cyan << '|' << ansi::reset << '\n';
    }
};

} // namespace dronecontrol::ui
