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

---

## 4. Native macOS (Apple Silicon) Build
You can also build and run the native macOS ARM64 executable (`SCDA`) using CMake and Ninja:

```bash
cmake -B build-macos -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build-macos SCDA
./build-macos/SCDA
```
