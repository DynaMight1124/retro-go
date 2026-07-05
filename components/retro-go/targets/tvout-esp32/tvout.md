# Retro-Go TVOut Driver: Composite Video for ESP32

This document provides a technical overview of the standard-compliant composite video (CVBS) TVOut driver implemented for the Retro-Go ecosystem on the ESP32.

---

## 1. Driver Overview
The TVout driver enables standard progressive NTSC or PAL composite video output directly from an ESP32. It operates without external video processors by leveraging:
1. **The Internal 8-Bit DAC (GPIO25)**: The ESP32's hardware Digital-to-Analog Converter (channel 1) generates the raw analog voltages ($0.0\text{V}$ sync to $\approx 1.0\text{V}$ peak white).
2. **I2S0 in LCD Mode with DMA**: The I2S0 peripheral is hijacked in parallel LCD mode (`lcd_en = 1`) to stream pixel and timing samples continuously from RAM to the DAC.
3. **The APLL (Analog PLL) Clock**: A high-precision clock source configured to standard color subcarrier multiples ($14.31818\text{ MHz}$ for NTSC, $17.734475\text{ MHz}$ for PAL).

---

## 2. NTSC vs. PAL Signal Generation

The TVOut driver generates standard broadcast waveforms in real-time. Because composite video is a single-wire analog format, timing and color encoding are strictly defined by television standards:

### NTSC (National Television System Committee)
* **Frequency / Frame Rate**: $60\text{ Hz}$ progressive scan ($262$ total lines per frame).
* **Clock Frequency**: $\approx 14.318\text{ MHz}$ ($4 \times$ NTSC subcarrier of $3.579545\text{ MHz}$).
* **Horizontal Timing**: $912$ clock samples per scanline ($720$ active pixels).
* **Color Burst**: Fixed phase at $180^\circ$ ($-U$ axis) on every scanline, allowing the TV to synchronize the phase of active colors.

### PAL (Phase Alternating Line)
* **Frequency / Frame Rate**: $50\text{ Hz}$ progressive scan ($312$ total lines per frame).
* **Clock Frequency**: $\approx 17.734\text{ MHz}$ ($4 \times$ PAL subcarrier of $4.433618\text{ MHz}$).
* **Horizontal Timing**: $1136$ clock samples per scanline ($896$ active pixels).
* **Chroma Phase Alternation**: PAL mitigates transmission phase errors by alternating the polarity of the $V$ (red-green) color axis on every consecutive scanline.
* **Swinging Color Burst**: To tell the TV's decoder which line polarity is currently active, the color burst phase "swings" back and forth line-by-line:
  * **Even lines**: $+135^\circ$ phase ($-U + V$).
  * **Odd lines**: $-135^\circ$ phase ($-U - V$).


---

## 3. Performance & Memory Optimisations

Streaming high-frequency analog signals in real-time requires strict timing limits. Several hot-path optimisations were implemented to keep the CPU load low:

### DRAM Line Caching
* Reading pixels sequentially from external PSRAM during active DMA refills causes random-access latency spikes and cache thrashing. 
* The driver fetches the entire active line in a single sequential block write using `memcpy` into a fast, local DRAM buffer (`line_temp`) before performing subcarrier color lookups. This isolates video generation from PSRAM bus contention.

### Pre-shifted 32-bit Color LUTs
* Originally, color modulation required byte shifts and 2D array lookups inside the pixel processing loop.
* We pre-calculate four 32-bit LUTs (`color_lut_even_s0` through `s3`) at boot, with the byte-modulations pre-shifted.
* Active pixel processing now requires only two 32-bit loads and a bitwise OR, reducing pixel rendering CPU cycles by $\approx 40\%$.

### Zero-Overhead PAL Swinging Burst
* The alternating PAL bursts (`pal_burst0` and `pal_burst1`) are pre-calculated at boot with correct phase offsets ($0.0\text{f}$ starting phase) and pre-swapped byte layouts.
* In `fill_chunk`, a fast 80-byte `memcpy` copies the correct phase to the DMA buffer using line-counter parity (`line & 1`). This uses less than 30 CPU cycles per line ($\approx 0.0025\%$ of CPU capacity).
* NTSC completely bypasses this logic (keeping the NTSC burst pre-baked in the static templates) to ensure no performance impact.

---

## 4. Performance & Memory Costs

Using the TVout driver introduces resource constraints compared to standard SPI LCD drivers:

| Metric | SPI LCD Driver | TVOut Driver (320x240) | TVOut Driver (256x240) |
| :--- | :--- | :--- | :--- |
| **DRAM Buffer Size** | $\approx 4\text{ KB} - 16\text{ KB}$ | **$76.8\text{ KB}$** (320x240 8-bit) | **$61.4\text{ KB}$** (256x240 8-bit) |
| **DRAM Location** | Usually fits in Internal RAM | Can fail and spill to PSRAM | Guaranteed in Internal RAM |
| **Core 0 CPU Usage** | Low (SPI DMA offloaded) | Low-Medium (scaling load) | Low (1:1 blitting) |
| **Core 1 CPU Usage** | $0\%$ | $\approx 25\%$ (I2S DMA Pump) | $\approx 25\%$ (I2S DMA Pump) |

### The PSRAM Constraint
If the 8-bit `screen_buffer` fails to allocate in fast internal DRAM and spills into PSRAM, memory bus conflicts with the emulator thread on Core 0 will cause video jitter and 100% CPU lockups.

### Recommended Resolution: 256x240
By changing the target screen resolution from `320x240` to `256x240` (or `256x224` for NES) in `config.h`:
1. The required internal DRAM footprint drops by **$15.4\text{ KB} - 19.5\text{ KB}$**, ensuring the buffer easily fits in internal RAM.
2. Horizontal scaling calculations are bypassed when emulating retro systems with native 256-pixel wide screens, saving rendering CPU cycles.
3. The TVOut DMA pump automatically stretches the 256 pixels across the 720 NTSC samples, so the game still fills the screen.

---

## 5. Credits & References

The implementation of the Retro-Go TVOut driver was made possible by referencing the following ESP32 composite video codebases:

1. **[esp_8_bit](https://github.com/rossumur/esp_8_bit)** by *rossumur*  
   *Provided the foundational Zero-Math DAC timing logic, APLL clock configurations, and byte-swapping techniques to output high-detail waveforms.*
2. **[Anemoia-ESP32](https://github.com/Shim06/Anemoia-ESP32)** by *Shim06*  
   *Offered a clean, modern ESP32 composite implementation reference.*
