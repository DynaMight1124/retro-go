# Retro-Go Bootstrap Flasher

An application for Retro-Go that allows you to dynamically flash and boot either official Retro-Go binary files or third-party native ESP32 binaries (e.g., custom games, utility apps, Arduino sketches) directly from your SD card without getting locked out of the Retro-Go launcher. Most useful for devices with limited 4MB flash, aswell as devices that have a lot of third party binary support. Note: The binary files need to be compatible with your actual device!

---

## The Problem & The Solution

Normally, ESP32 Retro-Go apps launch each other by updating the OTA data partition (`otadata`) and performing a software restart. Because standard third-party ESP32 applications do not contain Retro-Go framework code, they have no built-in way to reset the boot partition back to the launcher when you exit. If you boot into a standard third-party binary, you are stuck there; even powering the console off and on will simply boot you straight back into that same app, locking you out of Retro-Go.

**Retro-Go Bootstrap Flasher** solves this by combining a dynamic flasher application with a customised second-stage bootloader recovery hook. If you ever get stuck, **simply turn the physical power switch off and back on** to immediately return to your Retro-Go launcher!

---

## Features

*   💾 **Dynamic Binary Flashing**: Flashes ESP32 `.bin` files from your SD card directly to a dedicated target partition (`bootstrapped`) on the fly.
*   ⚡ **Smart Reflash Prevention**: Remembers the path of the last flashed file in the NVS namespace under the `LastFlashedApp` settings key. If you select the same game consecutively, it bypasses the slow erase/flash process and boots directly.
*   🎮 **Integrated Retro-Go UX**: Provides a native Retro-Go dialog prompt asking whether you want to **Boot now** (skip flashing and boot instantly) or **Reflash** (force-reflash the binary).
*   🛡️ **Bootloader Recovery Hook (Lockout Safe)**: Integrates a custom second-stage bootloader hook (`bootloader_after_init`) that intercepts cold boots. If you power-cycle the device (hardware power switch off/on), the hook detects it, erases `otadata`, and automatically redirects the device back to the Retro-Go launcher (OTA slot 0).


---

## How to Set Up Your SD Card

Retro-Go Bootstrap Flasher registers itself as the handler for `.bin` files placed in the `apps` directory:

1. Create a folder named `apps` inside the `roms` directory of your SD card:
   ```text
   SD Card root/
   └── roms/
       └── apps/
           ├── Doom.bin
           ├── MiniTV.bin
           └── nes_emulator.bin
   ```
2. Place any native ESP32 `.bin` file in the `/roms/apps/` folder.
3. These applications will automatically appear under the **Apps** section in your Retro-Go launcher menu. Selecting one will seamlessly start the Bootstrap Flasher.

---

## How It Works

### 1. The Flashing Flow:
1. When you select a `.bin` file from the launcher, the launcher starts `bootstrap`.
2. `bootstrap` checks if the chosen file path matches the `LastFlashedApp` setting.
    *   **If it matches**: It prompts you with a Retro-Go styled dialog: `Already Flashed: This app was flashed previously. [ Boot now ] [ Reflash ]`.
    *   **If it doesn't match** (or you select *Reflash*): It erases the `bootstrapped` partition, reads the `.bin` block-by-block from the SD card, flashes it.
3. Once ready, `bootstrap` calls `esp_ota_set_boot_partition()` to set the active boot partition to the `bootstrapped` slot.
4. It triggers a clean software reboot (`esp_restart()`) to boot into your application.

### 2. The Power-Cycle Recovery Hook:
Because third-party binaries do not contain Retro-Go library code, they cannot write to `otadata` to return you to the launcher when you exit. To solve this, we implemented a custom hook inside the second-stage bootloader at `0x1000`:

*   **Software Reset**: If the reset was triggered by a software command (e.g., when `bootstrap` launches the game), the hook lets it boot into the game partition normally.
*   **Hardware Reset (Power Switch)**: If the device is physically powered off and on, the hook erases the `otadata` sector. The ESP32 bootloader then automatically defaults to booting OTA slot 0 (the Retro-Go `launcher`).

> [NOTE]
> The custom bootloader hook is implemented inside `launcher/bootloader_components/boot_hooks/hooks.c` and is statically compiled into the custom `bootloader.bin`. The linking phase uses `-Wl,--undefined=bootloader_hooks_include` to guarantee the GNU Linker does not optimise out these weak overrides.

---

## What Works Well

*   **Flashing standard ESP32 binaries**: Works flawlessly with standard ESP-IDF binaries and Arduino IDE compiled `.bin` outputs.
*   **Flash Wear Reduction**: Reduces write cycles on the ESP32's SPI flash by bypassing the erase/flash cycles for consecutive game sessions.
*   **Failsafe Recovery**: You can safely flash arbitrary binaries (even crashing or buggy code) because turning the power switch off and on always guarantees a safe return to the Retro-Go launcher.

---

## Limitations (What *Doesn't* Work Well)

*   **Size Constraints**: The binary size is strictly limited by the `bootstrapped` partition size. In standard 4MB layouts, this partition is configured as **1.0 MB** (`1048576` bytes) to leave enough space for the Retro-Go launcher, retro-core, and partition tables. Binaries exceeding this size cannot be flashed. Refer to the [Customising the Partition Size](#customising-the-partition-size) section below to adjust this limit.
*   **No Soft Exit in Third-Party Apps**: While Retro-Go cores support a virtual in-game exit menu to return to the launcher, third-party apps run natively and take complete control of the hardware. To exit a third-party app and return to the launcher, you **must physically toggle the power switch** (or press the hardware reset button).
*   **Hardware Conflict Risks**: If the third-party binary attempts to configure GPIO pins or peripherals (like screen controllers, SD card host, or audio DAC) in a way that conflicts with your target board's pin layout, the app may crash or display corrupted graphics. Ensure the third-party binary is compiled for your specific hardware.
*   **No Launcher Support**: Although works fine for some Retro-Go binaries such as Doom and some other ports, it will not work with emulators that rely on roms/games being selected via the launcher prior to starting the emulator.

---

## Customising the Partition Size

By default, the `bootstrapped` partition is configured with a size of **1.0 MB** (`1048576` bytes) to ensure the standard 4MB flash layout has plenty of room for all core Retro-Go applications and emulators. 

If you need to flash larger third-party applications, you can easily increase the partition size by editing `rg_tool.py`.

### How to Modify the Partition Size:
1. Open the [rg_tool.py] file in a text editor.
2. Search for the following line:
   ```
   args += ["0", str(ota_next_id), "1048576", "bootstrapped", "none"]
   ```
3. Change `"1048576"` to the decimal byte size corresponding to your desired target size:

| Desired Size | Decimal Value to Use in `rg_tool.py` | Notes / Hardware Guidance |
| :--- | :--- | :--- |
| **1.0 MB** | `"1048576"` | *(Default)* Safely fits all standard 4MB flash targets with core emulators. |
| **1.5 MB** | `"1572864"` | Should *just* fit standard 4MB flash targets when building firmware image (e.g., `launcher` `bootstrap` & `retro-core` only). |
| **2.0 MB** | `"2097152"` | Extremely tight on 4MB flash; requires compiling only the absolute barebones launcher and bootstrap. |
| **2.5 MB** | `"2621440"` | Ideal for 8MB or 16MB flash targets. |
| **3.0 MB** | `"3145728"` | Ideal for 8MB or 16MB flash targets. |
| **4.0 MB** | `"4194304"` | Recommended only for 8MB or 16MB flash targets. |

> [WARNING]
> If you are using a **4MB flash** device, your total firmware image size cannot exceed 4MB (4,194,304 bytes). If you set the `bootstrapped` partition to **1.5MB** or **2.0MB**, you must compile a lightweight image to avoid partition overlap or compile errors.

---



## Installation & Compilation

To build a lightweight, 4MB-flash-compliant Retro-Go image containing `launcher`, `retro-core`, and `bootstrap`, run:

```bash
python rg_tool.py --target cyd build-img launcher retro-core bootstrap
```

This will produce the image file with a footprint of only **3.375 MB**.


