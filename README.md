# Splinter Cell: Double Agent — Apple Silicon (macOS & iOS / iPadOS) Port

Static XBE recompilation of *Tom Clancy's Splinter Cell: Double Agent* (Original Xbox) ported natively to **Apple Silicon (macOS)** and **iOS / iPadOS** using **SDL3**, **Vulkan / MoltenVK (Metal)**, and **ImGui**.

## Architecture & Rendering Pipeline
```text
Original Xbox x86 (.xbe) ──► Static Recompilation (ARM64 C++)
Xbox D3D8 / NV2A GPU     ──► Vulkan 1.3 Backend ──► MoltenVK ──► Apple Metal (iOS / iPadOS / macOS)
Xbox DirectSound / Input ──► SDL3 Audio & Touch / Gamepad Layer
```

## Key Features on iOS & iPadOS
* **Clean Metal / MoltenVK Rendering:** Fixes bullet-hole decal borders and edge streaking (`greyborder2d`), NV2A Register Combiner `mux` blending, slope-scaled shadow acne, and dynamic vertex stutter (`16 MB` ring).
* **Native ARM64 Execution & Metal Rendering:** Runs at 60 FPS on Apple Silicon devices (A13 Bionic and newer, M1/M2/M3/M4 iPads) via MoltenVK.
* **Seamless In-Process Mission Transitions:** Custom `PerformSoftRelaunch()` architecture handles menu-to-mission and mission-to-menu reloads inside a single iOS process—compatible with **SideStore, AltStore, FlairStore, Feather, TrollStore, and LiveContainer**.
* **Customizable Touch Controls & Layout Editor:** Ergonomic iPhone & iPad button scaling, symmetrical `ABXY` layout, top-screen `MENU` and `HIDE/SHOW` pills, full ImGui touch navigation, and an interactive **Drag & Drop + Per-Button Resize** editor.
* **External or Bundled Game Assets:** Supports both a lightweight `~45 MB` IPA (`Documents/assets` via Apple **Files**) and a self-contained full IPA.

## Documentation
* **[Current Status & Roadmap](CURRENT_STATUS.md)** — Detailed breakdown of what works, disabled features (microphone), known limitations, and planned touch UI redesign.
* **[Building from Source](BUILDING.md)** — Full step-by-step instructions to extract Xbox assets, generate the Xcode project, and compile `.ipa` packages.
* **[Player & Installation Guide](USER_GUIDE.md)** — Sideloading instructions, touch layout editor guide, and troubleshooting.
* **[iOS & iPadOS Technical Port Fixes](IOS_PORT_FIXES.md)** — Architecture notes, Jetsam RAM optimizations, MoltenVK decal shader fixes, and touch UI details.
* **[Build & Sideloading Guide](BUILD_AND_SIDELOAD.md)** — Step-by-step instructions to build the `.ipa` and install via SideStore, AltStore, FlairStore, or LiveContainer.

> **Legal Notice:** This repository contains no copyrighted game assets (`default.xbe`, `.umd`, `.lin`, `.ss2`, `.bik`). You must provide your own legally dumped retail Xbox copy of *Tom Clancy's Splinter Cell: Double Agent*.
