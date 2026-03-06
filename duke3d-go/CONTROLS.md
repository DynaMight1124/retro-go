# Retro-Go Duke3D Controls

This document outlines the gamepad mapping and the specialized hotkey system implemented for Duke3D on Retro-Go compatible devices.

## Primary Controls (Game Mode)

| Retro-Go Button | Duke3D Action |
| :--- | :--- |
| **D-Pad UP** | Move Forward |
| **D-Pad DOWN** | Move Backward |
| **D-Pad LEFT** | Turn Left |
| **D-Pad RIGHT** | Turn Right |
| **A** | Fire Weapon |
| **B** | Jump |
| **X** | Crouch |
| **Y** | Toggle Jetpack |
| **L (Shoulder)** | Strafe Left |
| **R (Shoulder)** | Strafe Right |
| **SELECT** | Cycle Weapon |
| **START** | **Use / Open / Flip Switch** (Triggered on short-press release) |
| **MENU** | Toggle Game Menu |
| **OPTION** | Crouch |

---

## Shift Mode (Hold START for 1 Second)

Holding the **START** button transforms the D-pad into an inventory management tool. Movement is preserved during the initial hold to ensure smooth gameplay.

| START + D-Pad | Duke3D Action |
| :--- | :--- |
| **START + UP** | **Use Inventory Item** (Medkit, Steroids, etc.) |
| **START + DOWN** | **Toggle Jetpack** |
| **START + LEFT** | **Previous Inventory Item** |
| **START + RIGHT** | **Next Inventory Item** |

---

## How the Hotkey System Works

### 1. Transparent Hold
To ensure movement feels fluid, the game **does not stop** your current action the moment you press START. You can continue running or turning while preparing to use a hotkey. The D-pad only switches from "Movement" to "Inventory" once the 1-second timer has expired and Shift Mode becomes active.

### 2. Reliable "Sticky" Use (Short Press)
Activating doors or switches in Duke3D requires the "Use" key to be held for at least one game tick. 
- If you tap START and release it in **less than 1 second**, the system triggers a pulse.
- This pulse is **"Sticky"**: it automatically holds the Use action for **100ms** in the background. This ensures that every interaction is registered reliably by the game engine, even with very quick taps.

### 3. Input Snapshotting
The controller state is snapshotted at the beginning of every frame. This eliminates input "jitter" and ensures that complex combinations (like strafe-jumping while cycling inventory) remain perfectly stable across all ESP32 targets.
