#!/usr/bin/env python3
"""
DroneControlSuite — Integration Test Runner
Runs SITLTelemetryTest and displays results in a readable TUI.
Usage:
    python tools/run_tests.py
    python tools/run_tests.py --build       # cmake build first
    python tools/run_tests.py --filter HUD  # only tests whose name contains HUD
"""

import argparse
import os
import re
import subprocess
import sys
import time

# ---------------------------------------------------------------------------
# ANSI helpers
# ---------------------------------------------------------------------------

RESET  = "\033[0m"
BOLD   = "\033[1m"
DIM    = "\033[2m"
GREEN  = "\033[32m"
RED    = "\033[31m"
YELLOW = "\033[33m"
CYAN   = "\033[36m"
WHITE  = "\033[37m"
BG_GREEN = "\033[42m"
BG_RED   = "\033[41m"

def _enable_ansi_windows():
    if sys.platform == "win32":
        import ctypes
        kernel32 = ctypes.windll.kernel32
        kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
        # Force UTF-8 output
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")

def c(color, text):
    return f"{color}{text}{RESET}"

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------

SCRIPT_DIR   = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
BUILD_DIR    = os.path.join(PROJECT_ROOT, "build")

def find_binary():
    candidates = [
        os.path.join(BUILD_DIR, "SITLTelemetryTest.exe"),
        os.path.join(BUILD_DIR, "SITLTelemetryTest"),
        os.path.join(BUILD_DIR, "Release", "SITLTelemetryTest.exe"),
    ]
    for p in candidates:
        if os.path.isfile(p):
            return p
    return None

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------

def run_build():
    print(c(CYAN, "\n▶ Building SITLTelemetryTest..."))
    result = subprocess.run(
        ["cmake", "--build", BUILD_DIR, "--target", "SITLTelemetryTest", "--config", "Release"],
        cwd=PROJECT_ROOT,
    )
    if result.returncode != 0:
        print(c(RED, "✗ Build failed."))
        sys.exit(1)
    print(c(GREEN, "✓ Build succeeded.\n"))

# ---------------------------------------------------------------------------
# Output parser
# ---------------------------------------------------------------------------

RE_SUITE  = re.compile(r"^\[Test\]\s+(.+)$")
RE_PASS   = re.compile(r"^\s+\[PASS\]\s+(.+)$")
RE_FAIL   = re.compile(r"^\s+\[FAIL\]\s+(.+)$")
RE_RESULT = re.compile(r"=== Results: (\d+) passed, (\d+) failed ===")
RE_TELEM  = re.compile(r"^(Telemetry \|.+|HEARTBEAT.+|MissionControl.+|Waiting.+)$")

class TestResult:
    def __init__(self, name):
        self.name   = name
        self.passed = []
        self.failed = []
        self.start  = time.time()
        self.end    = None

    @property
    def ok(self):
        return len(self.failed) == 0 and len(self.passed) > 0

    @property
    def elapsed(self):
        t = (self.end or time.time()) - self.start
        return f"{t:.2f}s"

# ---------------------------------------------------------------------------
# Runner
# ---------------------------------------------------------------------------

WIDTH = 60

def separator(char="─"):
    print(c(DIM, char * WIDTH))

def header():
    print()
    separator("═")
    title = " DroneControlSuite — Integration Tests "
    pad = (WIDTH - len(title)) // 2
    print(c(CYAN, "═" * pad) + c(BOLD + CYAN, title) + c(CYAN, "═" * (WIDTH - pad - len(title))))
    separator("═")
    print()

def run_tests(binary, name_filter=None):
    suites     = []
    current    = None
    total_pass = 0
    total_fail = 0

    print(c(DIM, f"  Binary : {binary}"))
    print(c(DIM, f"  Filter : {name_filter or '(none)'}"))
    print()

    proc = subprocess.Popen(
        [binary],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )

    for raw in proc.stdout:
        line = raw.rstrip()

        # New test suite header
        m = RE_SUITE.match(line)
        if m:
            if current:
                current.end = time.time()
            current = TestResult(m.group(1).strip())

            # Apply filter
            if name_filter and name_filter.lower() not in current.name.lower():
                current = None
                continue

            suites.append(current)
            separator()
            print(c(BOLD + YELLOW, f"  ▶ {current.name}"))
            continue

        if current is None:
            continue

        # PASS line
        m = RE_PASS.match(line)
        if m:
            label = m.group(1).strip()
            current.passed.append(label)
            print(f"    {c(GREEN, '✓')} {label}")
            continue

        # FAIL line
        m = RE_FAIL.match(line)
        if m:
            label = m.group(1).strip()
            current.failed.append(label)
            print(f"    {c(RED, '✗')} {c(RED, label)}")
            continue

        # Telemetry noise — show dimmed
        if RE_TELEM.match(line):
            print(c(DIM, f"    {line}"))
            continue

    proc.wait()

    if current:
        current.end = time.time()

    # Collect totals from suites (filter may have skipped some)
    for s in suites:
        total_pass += len(s.passed)
        total_fail += len(s.failed)

    return suites, total_pass, total_fail, proc.returncode

def print_summary(suites, total_pass, total_fail, returncode):
    separator("═")
    print(c(BOLD, "\n  SUMMARY\n"))

    col_w = max((len(s.name) for s in suites), default=20) + 2
    for s in suites:
        status = c(BG_GREEN + BOLD, " PASS ") if s.ok else c(BG_RED + BOLD, " FAIL ")
        checks = f"{c(GREEN, str(len(s.passed)))}✓  {c(RED, str(len(s.failed)))}✗"
        print(f"  {status}  {s.name:<{col_w}} {checks}   {c(DIM, s.elapsed)}")

    print()
    separator("═")

    if total_fail == 0 and total_pass > 0:
        verdict = c(BG_GREEN + BOLD, f"  ALL {total_pass} CHECKS PASSED  ")
    else:
        verdict = c(BG_RED + BOLD, f"  {total_fail} FAILED / {total_pass + total_fail} TOTAL  ")

    pad = (WIDTH - len(f"  {total_pass + total_fail} TOTAL  ") - 4) // 2
    print(" " * pad + verdict)
    print()
    separator("═")
    print()

# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    _enable_ansi_windows()

    parser = argparse.ArgumentParser(description="DroneControlSuite integration test runner")
    parser.add_argument("--build",  action="store_true", help="cmake build before running")
    parser.add_argument("--filter", metavar="NAME",      help="only run tests whose name contains NAME")
    args = parser.parse_args()

    if args.build:
        run_build()

    binary = find_binary()
    if not binary:
        print(c(RED, "\n✗ SITLTelemetryTest binary not found."))
        print(c(DIM,  "  Run with --build or build manually:\n"
                      "  cmake --build build --target SITLTelemetryTest\n"))
        sys.exit(1)

    header()
    suites, total_pass, total_fail, rc = run_tests(binary, args.filter)
    print_summary(suites, total_pass, total_fail, rc)

    sys.exit(0 if total_fail == 0 else 1)

if __name__ == "__main__":
    main()
