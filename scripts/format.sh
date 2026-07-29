#!/usr/bin/env bash
set -e
find src include examples tests -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
