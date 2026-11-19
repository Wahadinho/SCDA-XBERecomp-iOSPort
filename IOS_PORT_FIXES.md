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
