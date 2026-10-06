# AGENTS.md — DroneControlSuite

Bu dosya, projeye dahil olacak yapay zeka sistemleri için bağlam ve talimatlar içerir.

---

## Projenin Amacı

DroneControlSuite, **ArduPilot/PX4 tabanlı insansız hava araçları (İHA/drone)** için modüler bir görev kontrol ve telemetri çerçevesidir. C++20 ile yazılmıştır.

Temel hedef: SITL simülasyonundan gerçek donanıma kadar ölçeklenebilen, bağımsız modüllerden oluşan bir UAV yazılım katmanı.

---

## Bağlantı Mimarisi

```
ArduPilot SITL
  ├── UDP 14550  →  Mission Planner (GCS — harici araç)
  └── UDP 14600  →  DroneControlSuite (bu proje)
```

Yerel simülasyon için: `python openclaw-sandbox/local_mavlink_sim.py`

---

## Mevcut Implementasyon Durumu

### Tamamlanan Bileşenler

| Bileşen | Dosya | Ne Yapar |
|---------|-------|----------|
| MavlinkTelemetryReceiver | `src/telemetry/MavlinkTelemetryReceiver.cpp` | UDP üzerinden MAVLink v1 paketleri alır ve parse eder |
| TelemetryDisplay | `include/ui/TelemetryDisplay.hpp` | ANSI renkli gerçek zamanlı konsol arayüzü (200ms) |
| SITL Integration Tests | `tests/integration/sitl_telemetry_integration_test.cpp` | 6 test / 23 check — gerçek UDP üzerinden |
| Test Runner UI | `tools/run_tests.py` | Python tabanlı renkli test denetleme arayüzü |
| CI/CD | `.github/workflows/ci.yml` | Ubuntu GCC/Clang + Windows MSVC matrix build |

### Boş Modüller (Yapılacak)

- `src/communication/` — MAVLink iki yönlü iletişim
- `src/mission/` — Waypoint tabanlı görev planlama
- `src/logger/` — Merkezi loglama
- `src/camera/` — Kamera entegrasyonu

---

## Kritik Teknik Bilgiler

### MAVLink v1 VFR_HUD Wire Format

```
offset  0: airspeed    (float)
offset  4: groundspeed (float)   ← state_.groundSpeed
offset  8: alt         (float)   ← DİKKAT: bu heading değil!
offset 12: climb       (float)   ← state_.climbRate
offset 16: heading     (int16)   ← state_.headingDeg
offset 18: throttle    (uint16)
```

**Hata geçmişi:** Offset 8'den int16 okumak her zaman 0 döndürür (alt float'un düşük 2 byte'ı).

### Windows Build Gereksinimleri

- `NOMINMAX` — `windows.h` öncesi tanımlanmalı (`std::max` makro çakışması)
- `Winsock2.h` — `windows.h` öncesi include edilmeli
- CMake: `find_package(Threads REQUIRED)` + `Threads::Threads` linkage

### Ubuntu CI Gereksinimleri

- `find_package(Threads REQUIRED)` + `target_link_libraries(... Threads::Threads)`
- ctest'te `-C Release` kullanılmaz (single-config Makefile generator)

### Port Tablosu

| Port | Kullanım |
|------|----------|
| 14550 | Mission Planner (GCS) |
| 14551 | SITL ikinci çıkış (MAVLink spec) |
| 14600 | local_mavlink_sim.py → DroneControlSuite |
| 14661 | Integration test portu (canlı portlarla çakışmaz) |

---

## Klasör Yapısı

```
DroneControlSuite/
├── include/
│   ├── telemetry/MavlinkTelemetryReceiver.hpp   ← getState() public
│   └── ui/TelemetryDisplay.hpp                   ← header-only UI
├── src/
│   ├── core/main.cpp                              ← receiver thread + display loop
│   └── telemetry/MavlinkTelemetryReceiver.cpp
├── tests/
│   └── integration/sitl_telemetry_integration_test.cpp
├── tools/
│   └── run_tests.py                               ← python test arayüzü
├── config/
│   ├── simulation.yaml                            ← SITL port konfigürasyonu
│   └── telemetry.yaml
├── .github/workflows/ci.yml
└── CMakeLists.txt
```

---

## Build & Çalıştırma

```bash
# Build
cmake -S . -B build
cmake --build build

# Testleri çalıştır
python tools/run_tests.py

# Simülatör + MissionControl (iki ayrı terminal)
python ../local_mavlink_sim.py
./build/MissionControl
```

---

## GitHub Issues

Proje görevleri ve bileşen açıklamaları GitHub Issues'ta belgelenmiştir:
- `#9` Overview (proje amacı ve mimari)
- `#1–#5` Mevcut bileşenler
- `#6–#8` Yapılacak görevler (Logger, Mission Manager, İki Yönlü MAVLink)

---

## Sonraki Adımlar (Öncelik Sırasıyla)

1. **Logger modülü** — tüm modüller için ortak altyapı
2. **MAVLink iki yönlü iletişim** — ARM/DISARM/TAKEOFF komutları
3. **Mission Manager** — waypoint yükleme ve görev akışı
4. **Unit testler** — `tests/unit/` hâlâ boş
5. **ROS2 entegrasyonu** — roadmap'te işaretli

---

## Stil ve Kurallar

- C++20 standard, `.clang-format` uyumlu
- RAII, `const` correctness, smart pointer tercih
- `config/` dizininden harici konfigürasyon
- Her modül bağımsız test edilebilir olmalı
- Yorum: sadece neden açık değilse yaz
