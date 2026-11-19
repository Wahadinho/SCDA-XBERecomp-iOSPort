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

# Building from Source — Splinter Cell: Double Agent (iOS & iPadOS)

This guide provides step-by-step instructions and exact commands to extract Original Xbox game files, compile the statically recompiled iOS ARM64 application using Xcode and CMake, and package unsigned `.ipa` archives on **macOS**.

---

## 1. Prerequisites (macOS)
- **Apple Silicon or Intel Mac** running macOS 13.0 (Ventura) or newer.
- **Xcode** (15.0 or newer) with the **iOS SDK** and Command Line Tools installed:
  ```bash
  xcode-select --install
  sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
  ```
- **Homebrew** packages:
  ```bash
  brew install git cmake ninja sdl3 glslang molten-vk vulkan-headers extract-xiso
  ```
- **Python 3** (included with macOS / Xcode Command Line Tools).
- **Original Microsoft Xbox Game Disc Image (`.iso` / `.xiso.iso`):**
  - Retail Original Xbox version of *Tom Clancy's Splinter Cell: Double Agent* (`default.xbe` SHA-1: `91290baf93c2f6ae735a3106c48734665c826d84`).

---

## 2. Clone the Repository & Extract Game Assets
This repository includes the pre-generated static C++ recompilation sources under `generated/game/`, so you do not need to run `xbe_recomp` during compilation. However, `assets/default.xbe` must be present before running CMake so the build system can embed and verify its SHA-1 checksum.

1. **Clone the repository:**
   ```bash
   git clone --recursive https://github.com/Wahadinho/SCDA-XBERecomp-iOSPort.git
   cd SCDA-XBERecomp-iOSPort
   ```
2. **Create the `assets/` folder and extract your Xbox `.iso`:**
   ```bash
   mkdir -p assets
   extract-xiso -d assets "/path/to/SplinterCellDoubleAgent.iso"
   ```
   *(If you already have extracted files, place `default.xbe` and the `Default/` folder directly inside `assets/`.)*
3. **Verify `default.xbe` checksum:**
   ```bash
   shasum assets/default.xbe
   ```

---

## 3. Compile & Package Unsigned `.ipa` Files (iOS & iPadOS)

### Option A: Automated Build & Packaging Script (Recommended)
The included script generates the Xcode project (`build-ios/scda.xcodeproj`), compiles the `SCDA` target in `Release` mode (`arm64`), generates Home Screen icons (`Assets.car`), enables iOS **Files** app sharing (`UIFileSharingEnabled`), and outputs `.ipa` packages directly to your Desktop:

```bash
chmod +x scripts/build_ios_ipa.sh
./scripts/build_ios_ipa.sh
```

**Output files on `~/Desktop/`:**
- **`SCDA_iOS_Lite.ipa` (~45 MB):** Lightweight executable and shaders. After sideloading, copy your `assets/` folder into the app's folder via the Apple **Files** app (**On My iPhone / iPad → Splinter Cell: Double Agent → assets**).
- **`SCDA_iOS_Unsigned.ipa` (~3.9 GB):** Full self-contained bundle with `assets/` embedded inside `SCDA.app/assets/` (Store mode `zip -0`), ready for direct sideloading (**FlairStore**, **Feather**, **TrollStore**, **LiveContainer**).

### Option B: Manual Build via CMake & `xcodebuild`
If you prefer running each step manually:

1. **Generate the Xcode project:**
   ```bash
   cmake -B build-ios -G Xcode \
     -DCMAKE_SYSTEM_NAME=iOS \
     -DCMAKE_OSX_SYSROOT=iphoneos \
     -DCMAKE_OSX_ARCHITECTURES=arm64 \
     -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
   ```
2. **Compile `SCDA.app` (unsigned):**
   ```bash
   xcodebuild -project build-ios/scda.xcodeproj \
     -target SCDA \
     -configuration Release \
     -sdk iphoneos \
     -arch arm64 \
     CODE_SIGN_IDENTITY="" \
     CODE_SIGNING_REQUIRED=NO \
     CODE_SIGNING_ALLOWED=NO \
     build
   ```
3. **Package into `.ipa`:**
   ```bash
   rm -rf /tmp/ipa_build && mkdir -p /tmp/ipa_build/Payload
   cp -R build-ios/Release-iphoneos/SCDA.app /tmp/ipa_build/Payload/
   # Optional: include game assets for the full bundle
   cp -R assets /tmp/ipa_build/Payload/SCDA.app/
   (cd /tmp/ipa_build && zip -0 -qry "$HOME/Desktop/SCDA_iOS_Unsigned.ipa" Payload)
   ```
# iOS & iPadOS Port Technical Documentation — Splinter Cell: Double Agent (XBE Recomp)

This document details every architectural, memory, audio, rendering, UI, and packaging modification implemented to run the static recompilation of *Tom Clancy's Splinter Cell: Double Agent* natively on **iOS and iPadOS** across all sideloading methods (SideStore, AltStore, FlairStore, Feather, TrollStore, and LiveContainer).

---

## 1. Universal In-Process Soft-Relaunch (`PerformSoftRelaunch`)
* **Problem:** On Xbox, transitioning between the front-end menu (`Menu.lin`) and missions (`x01_iceland_a`, etc.) calls `HalReturnToFirmware(2)` (`XLaunchNewImageA`), which restarts the executable with `launch.bin`. On iOS, `execl()` is blocked by the sandbox and `std::_Exit(0)` terminates the app, while custom URL schemes (`scda://`) only work inside LiveContainer and fail on standalone sideloaders.
* **Solution (`src/kernel/boot.cpp`, `src/app/main.cpp`):**
  * Replaced process termination on iOS with an **in-process soft-relaunch** state machine (`kernel::g_soft_relaunch` + `kernel::PerformSoftRelaunch()`).
  * When `Relaunch()` is triggered, guest threads immediately exit cleanly at synchronization points (`dispatch.cpp`, `d3d8.cpp`, `xboxkrnl.cpp`, `communicator.cpp`).
  * The main UI/render loop executes `PerformSoftRelaunch()`, resetting:
    * `g_xbe = xbe::Image{}` before parsing (preventing section/library duplication from 34 to 68 sections).
    * `mem::Reset()` (`src/kernel/memory.cpp`) — clears all committed guest pages outside the physical window (`!InPhysical`), including `kSystemBase` (`0xB0000000+`) and `kStackBase` (`0xD0000000+`), while rotating a `32 MB` allocation base offset (`g_sys_relaunch_offset`) so persistent host pointers (`render_thread` ring buffers, static UI blocks) never collide across relaunches.
    * Subsystem states: `xapi::ResetState()`, `communicator::ResetState()`, `dsound::ResetState()`, `d3d::ResetState()` (including `g_up_ring`, `g_up_pos`, `g_up_header`), `xnet::CloseSockets()`, `ResetThreads()`, and `ResetObjects()`.

## 2. iOS Jetsam Memory Limit Optimizations
* **Problem:** Standard iOS sideload certificates (AltStore, SideStore, FlairStore) enforce a strict per-process Jetsam memory ceiling (~2.0–2.8 GB) without the *Increased Memory Limit* entitlement. Allocating 64 MB stacks per guest thread (~20 threads = 1.28 GB) plus `vm_allocate` for 4 GB virtual address space triggered immediate Jetsam kills after the intro videos.
* **Solution (`src/kernel/threads.cpp`, `src/kernel/memory.cpp`):**
  * Reduced guest `pthread` stack size on iOS from `64 MB` (`64u << 20`) to **`4 MB` (`4u << 20`)**, saving over **1.2 GB** of virtual/committed memory.
  * Switched the 4 GB guest address space reservation on iOS exclusively to `mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANON | MAP_NORESERVE, -1, 0)` so uncommitted pages consume zero Jetsam memory budget.

## 3. Removal of Microphone & Voice Link (TCC Privacy Crash Fix)
* **Problem:** Initializing Xbox Communicator voice capture (`SDL_OpenAudioDevice` for recording) on standalone iOS apps without `NSMicrophoneUsageDescription` causes an immediate OS-level TCC privacy termination (`SIGABRT`), and prompting users for microphone access is unnecessary for single-player/local play.
* **Solution (`src/audio/voice.cpp`, `src/audio/voice_link.cpp`):**
  * Disabled microphone enumeration and capture on iOS (`OpenMicrophone` returns `nullptr` -> `no device: silent`, `Microphones()` returns `{}`), and made `StartVoiceLink` / `StopVoiceLink` no-ops on iOS.

## 4. MoltenVK / Apple Silicon Graphics & Visual Glitch Fixes
* **Bullet-Hole Decals, Purple/Grey Flashes & Square Outline Artifacts (`src/gpu/shaders.cpp`, `src/gpu/vulkan/vulkan.cpp`):**
  * *Issue:* Apple GPUs under MoltenVK do not expose `VK_EXT_custom_border_color` (`custom_border_ == false`), causing Xbox `D3DTADDRESS_BORDER` textures with mid-grey border colors (`0x00808080`, used for `modulate2x` bullet decals and projected effects) to fall back to `TRANSPARENT_BLACK` (`0, 0, 0, 0`). This initially caused bright purple/grey flashes when shooting walls; switching samplers to `CLAMP_TO_EDGE` caused full-wall edge streaking lines, while raw `CLAMP_TO_BORDER` left a thin dark square outline around every bullet hole.
  * *Fix:* Flagged textures with mid-grey border colors (`32 < R,G,B < 224`) using `kTexGreyBorder` across both 2D and 3D texture stages and implemented `greyborder()` (`float4(v.rgb + 0.5 * (1.0 - v.a), v.a)`) plus **`greyborder2d()`** with a `[0.006 .. 0.994]` UV boundary guard returning `float4(0.5, 0.5, 0.5, 0.0)`. This completely eliminates purple flashes, wall edge-streaking lines, and dark square borders around bullet decals.
* **NV2A Register Combiner `mux` Restoration (`src/gpu/shaders.cpp`):**
  * *Issue:* Stripping the NV2A Register Combiner `mux` condition broke multi-stage surface blending, specular highlights, and lightmap selection on Metal/MoltenVK.
  * *Fix:* Restored the hardware-accurate NV2A Register Combiner `mux` selection logic in the shader translator.
* **Shadow Acne, Diagonal Stripes & Out-of-Frustum Sky Shadow Blocks (`src/gpu/shaders.cpp`, `src/gpu/vulkan/vulkan.cpp`):**
  * *Issue:* Fixed depth bias caused diagonal self-shadowing stripes across sloped terrain and character models, while out-of-frustum shadow lookups against MoltenVK's `0.0` border depth cast dark blocks across the sky.
  * *Fix:* Configured `VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE` (`1.0`) on shadow samplers, added explicit light-frustum bounds checks in `shadow()`, and applied a slope-scaled depth bias using screen-space depth derivatives (`ddx`/`ddy`).
* **Dynamic Vertex Ring Expansion (`src/xdk/d3d8.cpp`):**
  * *Fix:* Increased `kUpRing` from `4 MB` to **`16 MB` (`16u << 20`)** to eliminate mid-frame GPU fence stalls and micro-stuttering during `DrawVerticesUP` particle and decal draws.

## 5. Responsive iPhone & iPad Touch Controls + In-Game Layout Editor
* **Problem:** Scaling controls purely by screen height caused oversized buttons on iPad (`4:3` displays), `ABXY` buttons stretched vertically on non-16:9 screens, and `ImGui` menus did not receive touch coordinates.
* **Solution (`src/app/touch_controls.cpp`, `src/app/menu.cpp`):**
  * **DPI/Ergonomic Reference Sizing:** Clamped base reference height to `std::min(disp.y, 430.0f)` so controls maintain comfortable thumb ergonomics on both iPhone and iPad.
  * **Symmetrical `ABXY` Diamond:** Computed `A, B, X, Y` offsets in square `ref_h` units around the right-hand hub.
  * **Top Bar Pills (`MENU` / `HIDE` & `SHOW`):** Added two non-intrusive top-center pills to toggle the Recomp settings overlay or hide/show all virtual controls on demand.
  * **Direct ImGui Touch Forwarding:** Forwarded active finger coordinates and press states to `ImGuiIO` (`AddMousePosEvent` / `AddMouseButtonEvent`) whenever `MenuOpen()` is active.
  * **`TOUCH CONTROLS` Tab & Drag-and-Drop Editor:** Added a dedicated `TOUCH CONTROLS` tab inside `SETTINGS` with global scale, opacity, and an **Edit Layout** mode allowing users to **drag any button/stick anywhere on screen** and **resize individual controls** (`0.50x – 1.80x`), automatically persisted to `Documents/touch_layout.cfg`.

## 6. Display Defaults, Files.app Integration & Dual IPA Packaging
* **Exclusive Fullscreen (`src/app/app.h`, `src/app/settings.cpp`, `Info.plist`):** Defaulted iOS to `"fullscreen"` and `render_percent = 100` (balanced native sharpness and 60 FPS performance across iPhone and iPad) and set `UIStatusBarHidden = true` and `UIViewControllerBasedStatusBarAppearance = false`.
* **External `Documents/assets` Loading (`src/app/game_files.cpp`):** Updated `FindAssets()` to check `DataDir() / "assets"` (`Documents/assets`) first before checking the app bundle, enabling lightweight `~45 MB` `.ipa` installs (`SCDA_iOS_Lite.ipa`) where users copy game assets via Apple's **Files** app (`UIFileSharingEnabled = true`).


## Documentation
* **[Current Status & Roadmap](CURRENT_STATUS.md)** — Detailed breakdown of what works, disabled features (microphone), known limitations, and planned touch UI redesign.
* **[Building from Source](BUILD.md)** — Full step-by-step instructions to extract Xbox assets, generate the Xcode project, and compile `.ipa` packages.
* **[Player & Installation Guide](USER_GUIDE.md)** — Sideloading instructions, touch layout editor guide, and troubleshooting.
* **[iOS & iPadOS Technical Port Fixes](IOS_PORT_FIXES.md)** — Architecture notes, Jetsam RAM optimizations, MoltenVK decal shader fixes, and touch UI details.
* **[Build & Sideloading Guide](BUILD_AND_SIDELOAD.md)** — Step-by-step instructions to build the `.ipa` and install via SideStore, AltStore, FlairStore, or LiveContainer.

## Credits & Acknowledgements

This iOS & iPadOS port builds upon the foundational work of the following upstream projects and developers:

- **[TGP482/SCDA-XBERecomp](https://github.com/TGP482/SCDA-XBERecomp)** & **Joshhhuaaa** — Creators of `SCDA-XBERecomp` and `XboxXBE-Recomp`, designing the static recompilation framework, Xbox kernel/Direct3D8/DirectSound HLE layers, and the original PC port.
- **[aminerochdi1/SCDA-XBERecomp-MacOSPort](https://github.com/aminerochdi1/SCDA-XBERecomp-MacOSPort)** (**ROCHDI MOHAMMED AMINE**) — Native macOS Apple Silicon (ARM64) port, Darwin platform abstraction, SDL3 input/audio integration, and initial MoltenVK shadow fixes.
- **[mogeldev/SCDA-XBERecomp-android](https://github.com/mogeldev/SCDA-XBERecomp-android)** — Mobile ARM64 port architecture and on-screen touch control design inspiration.
- **Khronos Group & MoltenVK** — Vulkan-to-Metal translation layer enabling hardware-accelerated rendering on iOS and iPadOS.
- **libsdl-org / SDL3** — Cross-platform windowing, audio, gamepad, and multi-touch input handling.
- **Ubisoft Montreal** — Original creators of *Tom Clancy's Splinter Cell: Double Agent* (Original Xbox / Version 2).

## AI Assistance Disclaimer

Parts of the iOS/iPadOS platform adaptation, Objective-C++ / UIKit integration, touch-control overlay, build scripts, and documentation in this repository were developed with the assistance of AI coding tools. All generated code, build configurations, and runtime changes were manually reviewed, adapted, and tested on real Apple hardware (iPhone and iPad).

> **Legal Notice:** This repository contains no copyrighted game assets (`default.xbe`, `.umd`, `.lin`, `.ss2`, `.bik`). You must provide your own legally dumped retail Xbox copy of *Tom Clancy's Splinter Cell: Double Agent for Original Xbox (Xbox Classic)*.
