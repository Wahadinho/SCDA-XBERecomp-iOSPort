# User Guide — Splinter Cell: Double Agent (iOS & iPadOS)

## 1. Requirements
* **Device:** iPhone or iPad with Apple Silicon (A13 Bionic or newer; M1/M2/M3/M4 iPads recommended).
* **OS:** iOS 15.0 or iPadOS 15.0 and later.
* **Game Files:** Original Microsoft Xbox retail version of *Tom Clancy's Splinter Cell: Double Agent* (`default.xbe` SHA-1: `91290baf93c2f6ae735a3106c48734665c826d84`). PC, PS2, GameCube, and Xbox 360 versions are not supported.

---

## 2. Installing on iPhone & iPad

### Option A: SideStore / AltStore (Recommended)
1. Install **`SCDA_iOS_Lite.ipa`** using SideStore or AltStore.
2. Open the Apple **Files** app on your device and go to **On My iPhone (or iPad) → Splinter Cell: Double Agent**.
3. Copy your extracted **`assets`** folder (`default.xbe` + `Default/`) into this folder.
4. Launch the game from your Home Screen.

### Option B: FlairStore / Feather / TrollStore / LiveContainer
1. You may use either **`SCDA_iOS_Lite.ipa`** (with `assets/` placed in **Files.app**) or a self-built full **`SCDA_iOS_Unsigned.ipa`** (with `assets/` bundled inside `SCDA.app/`).
2. Sign and install with your preferred sideloading tool and launch from the Home Screen.

---

## 3. Using Touch Controls & Customizing Layout
* **`MENU` (Top-Center):** Opens the in-game Recomp Settings overlay.
* **`HIDE / SHOW` (Top-Center):** Hides or restores all on-screen virtual buttons and sticks.
* **Repositioning & Resizing Buttons:**
  1. Tap **`MENU`** → select the **`TOUCH CONTROLS`** tab.
  2. Adjust **`Global Scale`** or **`Button Opacity`**, or tap **`Edit Layout (Drag & Resize Buttons)`**.
  3. Drag any button or analog stick anywhere on the screen.
  4. Tap any control to select it (highlighted in green) and use the top **`Size`** slider (`0.50x – 1.80x`) to resize that individual button.
  5. Tap **`Done Editing`** to save your layout to `Documents/touch_layout.cfg`.
* **External Controllers:** Bluetooth Xbox, PlayStation (DualShock 4 / DualSense), and MFi controllers work automatically via SDL3. Tap **`HIDE`** on the top bar when playing with a physical gamepad.

---

## 4. Save Data & Configuration Location
All user files are accessible directly in the Apple **Files** app under **On My iPhone / iPad → Splinter Cell: Double Agent**:
* `assets/` — Your Xbox game files
* `hdd/UDATA/5553005e/` — Xbox save games and profiles
* `scda.toml` — Graphics, audio, and display settings
* `touch_layout.cfg` — Saved custom touch control positions and sizes
* `shadercache/` — Compiled Metal/Vulkan shader cache
* `logs/scda.log` — Diagnostic runtime log

---

## 5. Troubleshooting
* **App closes immediately on launch:** Verify that `assets/default.xbe` and `assets/Default/` are placed directly inside **On My iPhone / iPad → Splinter Cell: Double Agent → assets**.
* **Stuttering during first playthrough:** Metal shaders compile on first encounter and are saved to `shadercache/`. Subsequent runs will be smooth.
* **Crashes on 4 GB RAM devices (iPhone 11 / iPad 9/10):** Close all background apps before launching, set `Render Resolution` to `75%–85%`, and keep `MSAA` off.
