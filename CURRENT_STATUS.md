# Current Status & Roadmap — Splinter Cell: Double Agent (iOS & iPadOS)

> **The single-player campaign is playable end-to-end!** Menus, level transitions, saving/loading, 6-channel audio, and Metal hardware-accelerated rendering run natively at 60 FPS on Apple Silicon iPhones and iPads.

---

## 1. What Works (Fully Functional)
* **Native ARM64 Static Recompilation:** Over 33,000 original Xbox x86 functions translated ahead-of-time into native 64-bit ARM code running directly on iOS / iPadOS without JIT or virtualization.
* **Hardware-Accelerated Metal Rendering (via MoltenVK):** Full 3D rendering at up to 60 FPS with Night Vision, Thermal Vision, EEV, dynamic lighting, slope-scaled soft shadows, and clean bullet-hole decals (`greyborder2d`).
* **Seamless In-Process Mission Transitions (`PerformSoftRelaunch`):** Transitioning from the main menu into any campaign mission, advancing across mission parts, and returning to the main menu works smoothly inside a single iOS process across **SideStore, AltStore, FlairStore, Feather, TrollStore, and LiveContainer**.
* **6-Channel Audio & FMV Playback:** Full 48 kHz multi-channel audio via SDL3, including Bink (`.bik`) intro/cutscene movies and Xbox `.ss2` / `.ls2` sound banks.
* **Sideload-Safe Jetsam Memory Footprint:** Optimized `4 MB` per-thread stack allocation and `MAP_NORESERVE` virtual address space reservation allow the game to run reliably under standard sideload certificates without the *Increased Memory Limit* entitlement.
* **Interactive Touch Controls & Physical Gamepads:**
  * Functional on-screen virtual gamepad with symmetrical `ABXY` layout, top-center **`MENU`** and **`HIDE / SHOW`** pills, full touch navigation inside the ImGui settings overlay, and an in-game **Drag & Drop + Per-Button Size** layout editor (`Documents/touch_layout.cfg`).
  * Full plug-and-play support for Bluetooth Xbox, PlayStation (DualShock 4 / DualSense), and MFi controllers via SDL3.
* **Files.app Integration & Save Data:** Full read/write access via the Apple **Files** app (`UIFileSharingEnabled`), supporting external `Documents/assets` loading (`SCDA_iOS_Lite.ipa`) and persistent Xbox saves (`Documents/hdd/UDATA/5553005e/`).

---

## 2. What Is Currently Disabled / Known Limitations
* **Microphone / Xbox Communicator Voice Input (Disabled):**
  * Microphone capture (`OpenMicrophone` / `voice_link`) is intentionally disabled on iOS to prevent OS-level TCC privacy terminations (`SIGABRT`) on sideloaded builds and avoid unnecessary microphone permission prompts. Voice commands / headset voice chat are currently inactive.
* **Multiplayer (`SCDA_Versus`):**
  * The iOS build targets the single-player campaign executable (`default.xbe`). Online/LAN Versus multiplayer (`sc4_adv.xbe`) has not yet been packaged or tested on iOS.
* **4 GB RAM Devices Under Heavy Load:**
  * On devices with 4 GB total RAM (such as iPhone 11 / 11 Pro or entry-level iPads), background apps should be closed before launching and `Render Resolution` may need to be lowered to `75%–85%` during extended play sessions to stay within iOS Jetsam limits.

---

## 3. Roadmap (Planned Improvements & Fixes)
* [ ] **Redesign On-Screen Touch Controls Visual Appearance:** Replace the current basic geometric circles/lines with custom-designed, modern tactical icons, visual feedback animations, and polished skins matching the *Splinter Cell* aesthetic.
* [ ] **Optional Microphone Toggle (Safe Permission Handling):** Add an opt-in setting in `TOUCH CONTROLS` / `RECOMP SETTINGS` to safely request iOS microphone permission only when explicitly enabled by the user.
* [ ] **Shader Pre-Compilation / Reduced First-Run Stutter:** Bundle or pre-warm common Vulkan/MoltenVK pipeline states to eliminate minor shader compilation stutter on first playthrough.
* [ ] **iPad & Notch Safe-Area Auto-Refinement:** Further refine default button presets per device aspect ratio (iPhone notch/Dynamic Island vs. 4:3 iPad).
* [ ] **Multiplayer (`SCDA_Versus`) Support:** Investigate packaging and LAN/direct-IP testing for Versus mode on iOS/iPadOS.
