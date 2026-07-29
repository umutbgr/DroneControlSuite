# Architecture

DroneControl Suite follows a modular layered architecture designed for autonomous UAV mission software.

## Layers

1. **Core**: Application lifecycle, shared abstractions, and common utilities.
2. **Communication**: MAVLink and future external communication interfaces.
3. **Mission**: Mission planning, waypoint logic, and task execution flow.
4. **Telemetry**: Vehicle state tracking, validation, and telemetry processing.
5. **Camera**: Camera integration and future computer vision pipeline.
6. **UI**: User interaction, visualization, and mission control interfaces.

## Design Goal

The main goal is to keep each subsystem independent enough to test, replace, and extend without affecting the rest of the application.
