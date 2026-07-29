# ?? DroneControl Suite

A modular C++ mission planning and drone control framework designed for autonomous UAV operations.

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)
![CMake](https://img.shields.io/badge/Build-CMake064F8C)
![License](https://img.shields.io/badge/License-MIT-green)
![Status](https://img.shields.io/badge/Status-In%20Development-orange)

## ?? About

DroneControl Suite is a modular UAV mission planning and control framework written in Modern C++.

The project aims to provide a scalable architecture for integrating mission planning, telemetry, camera systems, MAVLink communication, and future ROS2/PX4 support.

## ? Features

- Modular architecture
- Mission planning
- MAVLink communication
- Telemetry management
- Camera module
- Logging system
- Configuration management
- Unit testing
- Cross-platform build with CMake
- Future ROS2 support
- Future PX4 integration

## ??? Tech Stack

- C++20
- CMake
- MAVLink
- Mission Planner
- ROS2 (planned)
- PX4 (planned)
- Git

## ??? Architecture

DroneControl Suite is organized as a layered UAV software framework. Each module has a clear responsibility and can evolve independently as the project grows.

```text
Application / UI Layer
        ?
Mission Management Layer
        ?
Telemetry & Camera Layer
        ?
Communication Layer
        ?
Core Utilities / Logging / Configuration
```

Detailed architecture documentation will be maintained in [`docs/architecture.md`](docs/architecture.md).

## ?? Project Structure

```text
DroneControlSuite/
??? .github/              # GitHub workflows and contribution templates
??? assets/               # Images, videos, logos, and demo media
??? config/               # Runtime configuration files
??? docs/                 # Architecture, module, and roadmap documentation
??? include/              # Public C++ headers organized by module
??? src/                  # Source files mirroring the include structure
??? tests/                # Unit, integration, and mock-based tests
??? third_party/          # External dependencies and vendor libraries
??? tools/                # Helper utilities for logs, missions, and telemetry
??? scripts/              # Build, run, and formatting scripts
??? examples/             # Example usage and sample integrations
??? CMake/                # Custom CMake modules
??? CMakeLists.txt        # Main CMake build configuration
??? README.md             # Project overview
??? LICENSE               # MIT license
??? CHANGELOG.md          # Version history
??? CONTRIBUTING.md       # Contribution guide
??? .gitignore            # Ignored build and IDE files
??? .clang-format         # C++ formatting rules
```

## ?? Installation

```bash
git clone https://github.com/umutbgr/DroneControlSuite.git
cd DroneControlSuite
mkdir build
cd build
cmake ..
cmake --build .
```

## ?? Usage

Linux/macOS:

```bash
./MissionControl
```

Windows:

```powershell
MissionControl.exe
```

## ?? Modules

### Core
Provides shared application primitives, lifecycle management, and common interfaces used across the framework.

### Communication Layer
Handles communication with external UAV systems and prepares the project for MAVLink-based integration.

### Mission Manager
Responsible for mission planning, waypoint handling, and autonomous task execution flow.

### Telemetry Manager
Collects, validates, and manages flight telemetry data such as position, altitude, attitude, and system status.

### Camera Manager
Provides a dedicated layer for camera integration, image acquisition, and future computer vision support.

### Logger
Centralized logging system for debugging, mission tracing, and runtime diagnostics.

### Config Manager
Loads mission, camera, and telemetry settings from external configuration files instead of hardcoded values.

### UI
Contains user-facing interfaces and future visualization components.

## ?? Engineering Decisions

### Why Modern C++?
Modern C++ provides performance, type safety, and strong control over system-level behavior, making it suitable for UAV mission software.

### Why CMake?
CMake enables cross-platform builds and provides a scalable build structure for future modules, tests, and external dependencies.

### Why Modular Architecture?
A modular structure keeps mission planning, telemetry, camera, communication, and UI logic separated. This improves maintainability and makes the system easier to extend.

### Why MAVLink?
MAVLink is a widely used communication protocol in UAV systems and provides compatibility with tools such as Mission Planner, PX4, and ArduPilot-based platforms.

### Why Not Python?
Python is excellent for prototyping, but this project focuses on building a performance-oriented and scalable mission software architecture using C++.

### Future Migration to ROS2
The current architecture is designed so that mission, telemetry, and communication modules can later be adapted into ROS2 nodes.

## ?? Roadmap

- ? Project structure
- ? Modular architecture
- ? Logging system
- ? ROS2 nodes
- ? PX4 integration
- ? Gazebo simulation
- ? QGroundControl support
- ? AI Vision module
- ? Multi-UAV support

## ?? Motivation

This project was developed to improve my understanding of autonomous UAV software architecture and modern C++ design while preparing for a career in mission software engineering.

## ?? What I Learned

- Modern C++
- Project architecture
- CMake
- Design patterns
- Modularity
- Mission planning concepts
- Software scalability

## ?? Future Work

- ROS2 integration
- PX4 support
- SLAM
- Computer vision
- Object tracking
- AI mission planning
- Cloud telemetry

## ?? Acknowledgements

- MAVLink
- PX4
- Mission Planner
- ROS2

## ?? License

This project is licensed under the MIT License. See [`LICENSE`](LICENSE) for details.

## ?? Contact

- GitHub: [umutbgr](https://github.com/umutbgr)
- LinkedIn: Add your LinkedIn profile here
- Email: Add your email address here
