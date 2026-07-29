# Contributing

Thank you for your interest in contributing to DroneControl Suite.

## Development Guidelines

- Use C++20 features where they improve clarity and safety.
- Keep modules separated by responsibility.
- Mirror public headers in `include/` with implementation files in `src/`.
- Avoid hardcoded runtime values; prefer files under `config/`.
- Format C++ code with the provided `.clang-format` file.

## Pull Request Checklist

- Code builds successfully with CMake.
- New logic is covered by tests where possible.
- Documentation is updated when architecture or behavior changes.
