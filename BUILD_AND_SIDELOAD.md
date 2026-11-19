# Building & Sideloading Guide (iOS / iPadOS)

## Prerequisites (macOS)
* **Xcode** (15.0 or newer) with iOS SDK installed
* **CMake** (3.24+) & **Python 3**
* Original Xbox retail files for *Splinter Cell: Double Agent* (`default.xbe` + `Default/` folder) placed inside `assets/`

---

## 1. Building the `.ipa` Packages
Run the included packaging script from the repository root:

```bash
# Generates build-ios/scda.xcodeproj via CMake (if missing), compiles arm64 Release, and packages the IPAs:
chmod +x scripts/build_ios_ipa.sh
./scripts/build_ios_ipa.sh
```

This generates two unsigned `.ipa` files on your Desktop:
1. **`~/Desktop/SCDA_iOS_Lite.ipa` (~45 MB):** Contains only the executable, shaders, and app icon (no `assets/`). Recommended for **SideStore** and **AltStore** to avoid ZIP64 large-archive extraction errors.
2. **`~/Desktop/SCDA_iOS_Unsigned.ipa` (~3.9 GB):** Full self-contained bundle with `assets/` packaged in Store mode (`zip -0`). Recommended for **FlairStore**, **Feather**, **TrollStore**, and **LiveContainer**.

---

## 2. Sideloading Instructions

### Method A: SideStore / AltStore (Recommended — Using `SCDA_iOS_Lite.ipa`)
1. Sign and install **`SCDA_iOS_Lite.ipa`** using SideStore or AltStore on your iPhone/iPad.
2. Open the Apple **Files** app on your device and go to **On My iPhone (or iPad) → Splinter Cell: Double Agent**.
3. Copy your `assets` folder (containing `default.xbe` and the `Default` folder) directly into that directory so the path is `Splinter Cell: Double Agent/assets/default.xbe`.
4. Launch the app from your Home Screen.

### Method B: FlareStore / Feather / TrollStore / LiveContainer (All-in-One IPA)
1. Import **`SCDA_iOS_Unsigned.ipa`** into your signing tool (FlairStore, Feather, ESign, TrollStore, or LiveContainer), sign with your certificate, and install.
2. Launch directly from the Home Screen — no extra file copying is required.

---

## 3. Touch Controls & In-Game Settings
* Tap **`MENU`** at the top-center of the screen to open the Recomp Settings overlay.
* Tap **`HIDE`** / **`SHOW`** at the top-center to hide or restore on-screen virtual controls at any time.
* Open **`MENU → TOUCH CONTROLS → Edit Layout`** to drag any button or analog stick to a custom position and adjust the **`Size`** slider for individual controls. Settings are saved automatically to `Documents/touch_layout.cfg`.
* On 4 GB RAM devices (e.g., iPhone 11 / 11 Pro), close background apps before launching and set `Render Scale` to `75%–85%` if needed.
