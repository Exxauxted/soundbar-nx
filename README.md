# Soundbar Mode — Tesla Overlay for Nintendo Switch

[![Platform](https://img.shields.io/badge/Platform-Nintendo%20Switch-e60012.svg)](https://www.nintendo.com/)
[![Framework](https://img.shields.io/badge/Framework-libtesla%20%7C%20libnx-blue.svg)](https://github.com/WerWolv/libtesla)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Release](https://img.shields.io/github/v/release/Exxauxted/soundbar-nx)](https://github.com/Exxauxted/soundbar-nx/releases/latest)

[**Русская версия (Russian)**](README_RU.md)

A Tesla / Ultrahand overlay (`.ovl`) for Horizon OS that forces audio output through the Nintendo Switch's **internal speakers while docked**, while continuing video output over HDMI. Turn your console into a compact soundbar!

---

## Features

- **Soundbar Mode Toggle**: Seamlessly route docked audio to the console's internal speakers.
- **Volume Slider**: 20-step discrete volume control (`StepTrackBar`) for internal speakers.
- **Instant Switching**: Zero-latency, zero-fade target switching (`fade_ns = 0`).
- **Game Stability Protection**: Carefully engineered to avoid HDMI audio DMA stalls and audio driver deadlocks.
- **Surround Detection**: Warns if TV audio output is set to 5.1ch Surround (which can cause buffer issues when downmixing).
- **Persistent Routing**: Audio routing remains active even after closing the overlay until reboot or manually toggled off.

---

## Prerequisites

To use this overlay, your Nintendo Switch must have Atmosphere CFW installed with the Tesla overlay environment:

| Component | Description |
|---|---|
| **Atmosphere CFW** | Custom firmware (1.0.0+) |
| **[nx-ovlloader](https://github.com/WerWolv/nx-ovlloader)** | Sysmodule that hosts overlays (`sd:/atmosphere/contents/420000000007E51A/`) |
| **[Tesla-Menu](https://github.com/WerWolv/Tesla-Menu)** or **[Ultrahand](https://github.com/ppkantorski/Ultrahand-Overlay)** | Overlay launcher menu (`sd:/switch/.overlays/ovlmenu.ovl`) |

---

## Installation

1. Download `soundbar-mode.ovl` from the [Latest Releases](https://github.com/Exxauxted/soundbar-nx/releases/latest).
2. Place `soundbar-mode.ovl` onto your Switch SD card into the overlays folder:
   ```
   sd:/switch/.overlays/soundbar-mode.ovl
   ```
3. Insert the SD card back into your Switch and boot Atmosphere.

---

## Usage

1. Put the console into the dock with HDMI connected to your TV / monitor.
2. Open the Tesla Menu using the default combination:  
   **`L` + `DPad Down` + `Right Stick Click (R3)`**
3. Select **Soundbar Mode**.
4. Toggle **Soundbar Mode** to **ON**.
5. Adjust the internal speaker volume using the slider.
6. Press **`B`** to close the overlay menu. Audio will continue playing through the internal speakers.

---

## ⚠️ Important: Preventing Game Freezes

To ensure smooth gameplay without micro-stutters or freezes while using Soundbar Mode:

1. **Set TV Sound to Stereo**:  
   Navigate to:  
   **System Settings → TV Output → TV Sound → select «Stereo»** (do **not** select «Surround» or «Automatic»).  
   *Why:* In docked mode, if Surround (5.1ch) is selected, games feed 6 uncompressed PCM audio channels to the system. Downmixing 6 channels to 2-channel internal speakers under heavy game load can lead to buffer underruns and game thread deadlocks.
2. **Close the Overlay with `B`**:  
   Always press **`B`** to fully exit the overlay back to the game instead of hiding it with the launch combo, allowing the overlay process to fully unload from RAM.

---

## How It Works (Technical Details)

Horizon OS manages audio routing via the `audctl` (Audio Controller) service with the following target IDs:

| ID | Target | Hardware Sink |
|:--:|---|---|
| `1` | `AudioTarget_Speaker` | Realtek ALC5640 / ALC5639 Codec (I2S) |
| `2` | `AudioTarget_Headphone` | 3.5mm Headphone Jack |
| `3` | `AudioTarget_Tv` | Tegra X1 HDMI Display Audio Packetizer |

When docked, Horizon OS normally forces `AudioTarget_Tv` as default.

### Key Implementation Principles

- **`audctlSetDefaultTarget(AudioTarget_Speaker, 0, 0)`**: Routes the master audio stream to the internal speakers cleanly without altering lower-level sink configurations.
- **TV is NOT Forcefully Muted**: Muting `AudioTarget_Tv` halts the HDMI audio FIFO DMA. If a running game maintains an active HDMI audio buffer queue, muting TV causes the audio renderer thread to block indefinitely waiting for buffer release (`svcWaitSynchronization`). Leaving TV unmuted while redirecting default target avoids this issue completely.
- **No `audctlSetOutputTarget` Conflict**: Avoids low-level hardware mux overrides that bypass the OS policy engine.

---

## Building from Source

### 1. Install devkitPro & switch-dev

```bash
# Arch Linux / pacman
sudo dkp-pacman -S switch-dev

# macOS (using Homebrew)
brew install devkitpro-pacman
sudo dkp-pacman -S switch-dev
```

### 2. Set Environment Variables

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITA64=$DEVKITPRO/devkitA64
export PATH=$DEVKITPRO/tools/bin:$DEVKITA64/bin:$PATH
```

### 3. Clone Repository with Submodules

```bash
git clone --recursive https://github.com/Exxauxted/soundbar-nx.git
cd soundbar-nx
```

### 4. Build

```bash
make
```

The resulting overlay binary will be at: `out/soundbar-mode.ovl`.

---

## Project Structure

```
soundbar-nx/
├── include/
│   └── audctl_ipc.hpp       # Safe C++ wrapper around libnx audctl API
├── source/
│   └── main.cpp             # Tesla overlay GUI & lifecycle implementation
├── lib/
│   └── libtesla/            # Header-only Tesla overlay framework (submodule)
├── Makefile                 # Build configuration compatible with switch_rules
├── LICENSE                  # MIT License
├── README.md                # English Documentation
└── README_RU.md             # Russian Documentation
```

---

## License

This project is licensed under the [MIT License](LICENSE).
