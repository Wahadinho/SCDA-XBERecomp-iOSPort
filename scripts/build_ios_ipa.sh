#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

if [ ! -d "build-ios/scda.xcodeproj" ]; then
  echo "==> Generating Xcode project in build-ios/..."
  cmake -B build-ios -GXcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_SYSROOT=iphoneos \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
fi

echo "==> Building SCDA for iOS (arm64 Release)..."
xcodebuild -project build-ios/scda.xcodeproj -target SCDA -configuration Release \
  -sdk iphoneos -arch arm64 \
  CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO build

rm -rf /tmp/ipa_build /tmp/AppIcon.xcassets
mkdir -p /tmp/ipa_build/Payload /tmp/AppIcon.xcassets/AppIcon.appiconset
cp -R build-ios/Release-iphoneos/SCDA.app /tmp/ipa_build/Payload/

if [ -f "icon.png" ]; then
  echo "==> Compiling iOS Home Screen icons (Assets.car)..."
  sips -s format jpeg -s formatOptions 100 icon.png --out /tmp/icon_flat.jpg >/dev/null
  sips -s format png /tmp/icon_flat.jpg --out /tmp/icon_flat.png >/dev/null
  python3 - << 'PY'
import json, subprocess, pathlib

flat = pathlib.Path("/tmp/icon_flat.png")
iconset = pathlib.Path("/tmp/AppIcon.xcassets/AppIcon.appiconset")
specs = [
    ("iphone", "60x60", "2x", 120),
    ("iphone", "60x60", "3x", 180),
    ("ipad", "76x76", "2x", 152),
    ("ipad", "83.5x83.5", "2x", 167),
    ("ios-marketing", "1024x1024", "1x", 1024),
]
images = []
for idiom, sz_str, scale, px in specs:
    fname = f"icon_{px}.png"
    subprocess.run(["sips", "-z", str(px), str(px), str(flat), "--out", str(iconset / fname)], stdout=subprocess.DEVNULL)
    images.append({"idiom": idiom, "size": sz_str, "scale": scale, "filename": fname})

(iconset / "Contents.json").write_text(json.dumps({"images": images, "info": {"version": 1, "author": "xcode"}}, indent=2))
(iconset.parent / "Contents.json").write_text(json.dumps({"info": {"version": 1, "author": "xcode"}}, indent=2))
PY
  xcrun actool /tmp/AppIcon.xcassets --compile /tmp/ipa_build/Payload/SCDA.app \
    --platform iphoneos --minimum-deployment-target 15.0 --app-icon AppIcon \
    --output-partial-info-plist /tmp/icon_plist.plist >/dev/null
  sips -z 120 120 /tmp/icon_flat.png --out /tmp/ipa_build/Payload/SCDA.app/AppIcon60x60@2x.png >/dev/null
  sips -z 180 180 /tmp/icon_flat.png --out /tmp/ipa_build/Payload/SCDA.app/AppIcon60x60@3x.png >/dev/null
  sips -z 152 152 /tmp/icon_flat.png --out /tmp/ipa_build/Payload/SCDA.app/AppIcon76x76@2x~ipad.png >/dev/null
  sips -z 167 167 /tmp/icon_flat.png --out /tmp/ipa_build/Payload/SCDA.app/AppIcon83.5x83.5@2x~ipad.png >/dev/null
fi

PLIST="/tmp/ipa_build/Payload/SCDA.app/Info.plist"
/usr/libexec/PlistBuddy -c "Delete :LSApplicationQueriesSchemes" "$PLIST" 2>/dev/null || true
/usr/libexec/PlistBuddy -c "Delete :NSMicrophoneUsageDescription" "$PLIST" 2>/dev/null || true
/usr/libexec/PlistBuddy -c "Add :UIFileSharingEnabled bool true" "$PLIST" 2>/dev/null || /usr/libexec/PlistBuddy -c "Set :UIFileSharingEnabled true" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :LSSupportsOpeningDocumentsInPlace bool true" "$PLIST" 2>/dev/null || /usr/libexec/PlistBuddy -c "Set :LSSupportsOpeningDocumentsInPlace true" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :UIStatusBarHidden bool true" "$PLIST" 2>/dev/null || /usr/libexec/PlistBuddy -c "Set :UIStatusBarHidden true" "$PLIST"
/usr/libexec/PlistBuddy -c "Add :UIViewControllerBasedStatusBarAppearance bool false" "$PLIST" 2>/dev/null || /usr/libexec/PlistBuddy -c "Set :UIViewControllerBasedStatusBarAppearance false" "$PLIST"
/usr/libexec/PlistBuddy -c "Merge /tmp/icon_plist.plist" "$PLIST" 2>/dev/null || true

echo "==> Packaging SCDA_iOS_Lite.ipa..."
(cd /tmp/ipa_build && zip -qry "$HOME/Desktop/SCDA_iOS_Lite.ipa" Payload)

if [ -d "assets" ]; then
  echo "==> Packaging SCDA_iOS_Unsigned.ipa (with assets)..."
  cp -R assets /tmp/ipa_build/Payload/SCDA.app/
  (cd /tmp/ipa_build && zip -0 -qry "$HOME/Desktop/SCDA_iOS_Unsigned.ipa" Payload)
fi

ls -lh "$HOME/Desktop"/SCDA_iOS_*.ipa
