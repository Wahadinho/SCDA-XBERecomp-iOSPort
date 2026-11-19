# Building from Source — Splinter Cell: Double Agent (iOS & iPadOS)

This guide provides step-by-step instructions and exact commands to extract Original Xbox game files, generate static C++ recompilation sources, compile the iOS ARM64 application using Xcode and CMake, and package unsigned `.ipa` archives on **macOS**.

---

## 1. Prerequisites (macOS)
* **Apple Silicon or Intel Mac** running macOS 13.0 (Ventura) or newer (16 GB+ RAM recommended for static recompilation).
* **Xcode** (15.0 or newer) with the **iOS SDK** and Command Line Tools installed:
  ```bash
  xcode-select --install
  sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
  ```
* **Homebrew** packages:
  ```bash
  brew install git cmake ninja sdl3 glslang molten-vk vulkan-headers
  ```
* **Python 3** (included with macOS / Xcode Command Line Tools).
* **Original Microsoft Xbox Game Disc Image (USA version of the game) (`.iso` / `.xiso.iso`):**
  * Retail Original Xbox version of *Tom Clancy's Splinter Cell: Double Agent* (`default.xbe` SHA-1: `91290baf93c2f6ae735a3106c48734665c826d84`).

---

## 2. Clone the Repository & Build Host Tools
First, clone the repository and compile the native macOS host utilities (`extract_xbe` for unpacking Xbox disc images and `xbe_recomp` for translating Xbox x86 machine code into C++).

1. **Clone the repository:**
   ```bash
   git clone --recursive https://github.com/Wahadinho/SCDA-XBERecomp-iOSPort.git
   cd SCDA-XBERecomp-iOSPort
   ```
2. **Build `extract_xbe` and `xbe_recomp`:**
   ```bash
   cmake -B build-tools -G Ninja -DCMAKE_BUILD_TYPE=Release
   ninja -C build-tools extract_xbe xbe_recomp
   ```
   *(Verify that `./build-tools/extract_xbe` and `./build-tools/xbe_recomp` exist before proceeding.)*

---

## 3. Extract Game Assets
Place your retail Xbox `.iso` image into the `assets/` folder and unpack the executable (`default.xbe`) along with the game data directories.

1. **Create the `assets` folder and copy your ISO:**
   ```bash
   mkdir -p assets
   cp "/path/to/SplinterCellDoubleAgent.iso" assets/
   ```
2. **Extract the Xbox ISO contents:**
   ```bash
   ./build-tools/extract_xbe "assets/<YOUR_ISO_FILENAME>.iso" assets
   ```
3. **Verification:** Ensure `assets/default.xbe` and the `assets/Default/` folder are extracted properly. You can verify the SHA-1 checksum with:
   ```bash
   shasum assets/default.xbe
   ```

---

## 4. Generate Xcode Project & Run Static Recompilation
Configure CMake for **iOS (`arm64`)** to generate `build-ios/scda.xcodeproj`. On the first configuration pass, `xbe_recomp` analyzes `assets/default.xbe` and outputs translated C++ source files into `generated/`.

```bash
cmake -B build-ios -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
```

> **Note on Memory Limits (`zsh: killed` / Exit Code 137):**
> Static recompilation and C++ compilation of 33,000+ functions can consume significant RAM. If `clang` is terminated by macOS (`Killed: 9`), close background applications and limit parallel compilation jobs by appending `-jobs 2` to `xcodebuild`.

---

## 5. Compile & Package Unsigned `.ipa` Files

### Option A: Automated Build Script (Recommended)
Run the included packaging script to compile the `Release` target, generate Home Screen icons (`Assets.car`), configure `Info.plist`, and output both `.ipa` variants on your Desktop:

```bash
chmod +x scripts/build_ios_ipa.sh
./scripts/build_ios_ipa.sh
```

### Option B: Manual Compilation & Packaging via `xcodebuild`
If you prefer building and packaging manually without the helper script:

1. **Compile the `.app` bundle (without code signing):**
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
2. **Package `SCDA_iOS_Lite.ipa` (Executable & Shaders only — ~45 MB):**
   ```bash
   rm -rf /tmp/ipa_build && mkdir -p /tmp/ipa_build/Payload
   cp -R build-ios/Release-iphoneos/SCDA.app /tmp/ipa_build/Payload/
   (cd /tmp/ipa_build && zip -qry "\$HOME/Desktop/SCDA_iOS_Lite.ipa" Payload)
   ```
3. **Package `SCDA_iOS_Unsigned.ipa` (Full self-contained bundle with game assets — Store mode `zip -0`):**
   ```bash
   cp -R assets /tmp/ipa_build/Payload/SCDA.app/
   (cd /tmp/ipa_build && zip -0 -qry "\$HOME/Desktop/SCDA_iOS_Unsigned.ipa" Payload)
   ```

**Output files:**
* `~/Desktop/SCDA_iOS_Lite.ipa`: Lightweight executable and shaders (copy `assets/` later via the Apple **Files** app — recommended for **SideStore** and **AltStore**).
* `~/Desktop/SCDA_iOS_Unsigned.ipa`: Full self-contained bundle ready for sideloading (**FlairStore**, **Feather**, **TrollStore**, **LiveContainer**).

---

## 6. Native macOS (Apple Silicon) Build
You can also compile and run the native macOS ARM64 build directly using CMake and Ninja:

```bash
cmake -B build-macos -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build-macos SCDA SCDA_Versus
cd build-macos && ./SCDA
```
