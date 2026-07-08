# Super Mario 64 Port for ESP32 (S3 & P4)

This project brings the classic Super Mario 64 to modern Espressif microcontrollers, specifically targeting the **ESP32-S3** and **ESP32-P4** chips using the **Retro-Go** framework.

## About the Port
This port is heavily based on the `sm64-funkey` codebase (which was based on the SM64-DOS codebase), particularly leveraging the software-rasterised renderer originally developed for the FunKey S device. Because the ESP32 series lacks a dedicated 3D hardware GPU with a standard graphics API (like OpenGL or Vulkan), this port uses a highly optimised 3D software rasterizer to draw polygons, apply textures, and handle Z-buffering purely on the CPU.

### Key Technical Achievements
*   **Architecture Agnostic Math:** The graphics pipeline was heavily debugged to run identically across both Xtensa LX7 (ESP32-S3) and RISC-V (ESP32-P4) architectures, fixing issues with matrix popping and floating-point alignment.
*   **Memory Optimisation:** The heavy texture cache and hashmap pools were re-engineered to run effectively within the PSRAM limits of the ESP32 without infinite traversal deadlocks.
*   **Asset Pipeline Independence:** The asset extraction toolchain has been decoupled from Linux binaries. Using native Python and the ESP-IDF compiler suite, assets can be compiled directly on Windows.

---

## Build Instructions

This project requires a legally dumped `baserom.us.z64` ROM of Super Mario 64. For legal reasons, no Nintendo IP or assets are included in this repository. **Note** Only the US rom has been tested and therefore supported.

### Prerequisites
1.  **ESP-IDF v5.5.1:** Ensure you have the ESP-IDF installed and your environment activated.
2.  **Docker:** Required *only once* to extract the raw asset images from the ROM. Windows users can install Docker Desktop.
3.  **Python 3:** Provided by your ESP-IDF environment. You will also need the `Pillow` library: `pip install Pillow`

### Step 1: Prepare the Base ROM
Place your `baserom.us.z64` file into the `components/sm64-funkey` directory.

### Step 2: Extract Raw Assets (via Docker)
Because the asset extraction tools (`n64graphics`, `mio0`) require a Linux C-compiler to build themselves, we use a lightweight Docker container to extract the raw images without messing up your host environment.

**Windows (PowerShell) / Linux / macOS:**
Navigate to `components/sm64-funkey` and run the extraction docker image:
```bash
cd components/sm64-funkey
docker run --rm -v ${PWD}:/sm64 ubuntu:20.04 /bin/bash -c "apt-get update && apt-get install -y build-essential python3 && cd /sm64 && make -C tools && python3 extract_assets.py us"
```
*(This will compile the extraction tools, rip the raw `.png` and `.aiff` files out of the ROM, and place them in the `textures/`, `levels/`, and `sound/` folders).*

### Step 3: Convert Assets to ESP32 Format (Native)
You do **not** need to compile the actual C code inside Docker.
From the same `components/sm64-funkey` directory on your host machine (with ESP-IDF activated), run the native Python preparation script:
```bash
python prepare_esp32_assets.py
```
This script handles the heavy lifting instantly:
*   Converts all `.png` files into 16-bit `rgba16.inc.c` arrays.
*   Slices the skyboxes and the ending cake into individual chunks.
*   Uses the local ESP-IDF C-Preprocessor (`xtensa-esp-elf-gcc`) to translate the text files into byte arrays.

### Step 4: Build the Game!
Return to the root directory of the repository (where `rg_tool.py` is located) and initiate the Retro-Go build process for your specific target.

For example, to build for the ESP32-P4 (e.g., GB300-P4):
```bash
python rg_tool.py --target gb300-p4 release launcher sm64-go
```
### Step 5: Run SM64
You need to place a .z64 file on your SD in `/roms/sm64`

This can be an empty file called `Super Mario 64.z64` for instance or you can just copy across `baserom.us.z64` the itself is only used to boot the game.


