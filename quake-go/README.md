# Quake-Go: Optimized Quake Port for Retro-Go

Quake-Go is a high-performance port of the legendary **Quake (WinQuake)** engine, modernised for ESP-IDF v5.5 and fully integrated into the **Retro-Go** ecosystem. This port features aggressive memory management and hardware-specific optimisations to deliver a playable experience on the ESP32-S3 and ESP32-P4 hardware.

---

## 🛠 Work Completed

- **ESP-IDF v5.5 Modernisation:** Fully updated for the latest framework, resolving legacy compiler warnings, linker conflicts, and API deprecations.
- **Cross-Generation Memory Architecture:**
  - Implemented a unified Hunk-based asset loader to resolve MMU exhaustion on lower-tier hardware.
  - Offloaded critical system overhead (Task Stack, Z-Buffer, Mix Buffers) to internal DRAM to maximize addressable PSRAM.
- **Unified Control Scheme:** 
  - **SELECT:** Weapon Toggle
  - **X / OPTION:** Swim Down
  - **Y:** Run / Walk Toggle
  - **START / MENU:** Quake Menu
- **Stability Fixes:** Implemented target-gated watchdog yields to prevent system resets during heavy asset loading on ESP32 hardware.

---

## 📱 Supported Hardware Comparison

### 1. ESP32-P4 
The powerhouse implementation. Leverages the massive resources of the P4 generation.
- **Resolution:** 320x240 (Full)
- **Hunk Size:** 8.0MB
- **Pros:** Peak framerate, high-quality audio, massive map support, no memory bottlenecks.
- **Cons:** Larger battery footprint.

### 2. ESP32-S3
The sweet spot for performance and portability.
- **Resolution:** 320x240
- **Hunk Size:** 6.0MB
- **Pros:** Stable performance, excellent display compatibility, standard resolution.
- **Cons:** PSRAM bandwidth can be a bottleneck in complex scenes.

### 3. Original ESP32 (Currently NOT working!)
A technical (but not working) breakthrough for the original architecture, utilizing aggressive memory offloading to break the 4MB PSRAM mapping limit. Its probably not far away from working, maybe 50-100kb here or there.
- **Resolution:** 160x120 (Internal software scaling) however may need lower
- **Hunk Size:** 3.4MB (Maximised)
- **Pros:** Very low hardware cost. Maybe a little too low!
- **Cons:** 4MB PSRAM and well, it doesnt work :)

---

## 📜 Credits & Licensing

This project is based on the amazing work originally developed by **thisiseth**. 

**Original Repository:** [tang-primer-25k-spi-io](https://github.com/thisiseth/tang-primer-25k-spi-io)

Quake is Copyright (C) 1996-1997 Id Software, Inc. This port is distributed under the terms of the GNU General Public License as published by the Free Software Foundation.

---

## 🚀 Getting Started

1. **PAK Files:** You must provide your own `pak0.pak` for shareware (and optionally `pak1.pak` for the full version).
2. **Storage:** Place PAK files in `/sd/roms/quake/` or `/sd/roms/quake/id1/`.
