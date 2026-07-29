# Mission Planner Simulation

This document describes how to run DroneControl Suite with a Mission Planner based simulation workflow.

## Goal

The goal is to test the project against a simulated UAV before using real hardware.

The recommended simulation setup is:

```text
ArduPilot SITL
     ├── UDP 127.0.0.1:14550 → Mission Planner
     └── UDP 127.0.0.1:14551 → DroneControl Suite
```

Mission Planner is used as the ground control station, while DroneControl Suite connects to a separate MAVLink UDP output for development and testing.

## Required Tools

- Mission Planner
- ArduPilot SITL
- Python 3
- Git
- CMake
- C++20 compiler

## Recommended UDP Ports

| Component | Protocol | Address | Port |
|---|---:|---|---:|
| Mission Planner | UDP | 127.0.0.1 | 14550 |
| DroneControl Suite | UDP | 127.0.0.1 | 14551 |

## SITL Startup Example

When using ArduPilot SITL, start the simulator with two MAVLink outputs:

```bash
sim_vehicle.py -v ArduCopter --console --map \
  --out=udp:127.0.0.1:14550 \
  --out=udp:127.0.0.1:14551
```

## Mission Planner Connection

1. Open Mission Planner.
2. Select `UDP` as the connection type.
3. Use port `14550`.
4. Connect to the running SITL instance.
5. Verify that attitude, GPS, mode, and telemetry values are visible.

## DroneControl Suite Connection

DroneControl Suite should use the second MAVLink output:

```yaml
connection: udp://127.0.0.1:14551
```

See [`config/simulation.yaml`](../config/simulation.yaml) for the recommended simulation configuration.

## Test Scenario

A basic validation scenario should include:

1. Start ArduPilot SITL.
2. Connect Mission Planner to UDP port `14550`.
3. Start DroneControl Suite and connect it to UDP port `14551`.
4. Verify heartbeat messages.
5. Verify telemetry reception.
6. Upload or define a simple waypoint mission.
7. Arm the simulated vehicle.
8. Switch to AUTO or GUIDED mode.
9. Monitor mission progress in Mission Planner.
10. Log telemetry and mission events inside DroneControl Suite.

## Expected Development Milestones

- [ ] Receive MAVLink heartbeat
- [ ] Parse vehicle attitude
- [ ] Parse GPS position
- [ ] Parse battery status
- [ ] Display telemetry in console
- [ ] Load mission from `config/mission.json`
- [ ] Send waypoint mission to simulated vehicle
- [ ] Track mission progress
- [ ] Log simulation session output

## Notes

This simulation workflow keeps Mission Planner and DroneControl Suite separated. Mission Planner remains the visual ground control station, while DroneControl Suite acts as an independent mission software client.
