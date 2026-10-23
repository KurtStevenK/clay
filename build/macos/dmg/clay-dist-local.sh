#!/bin/bash

# Turn the bundle made by 2_bundle-clay-uni_base.py (meson -Ddmg=true
# -Drelocatable-bundle=yes, then 'meson install') into a signed, notarized
# Clay DMG on a developer Mac. Unlike 3_dist-clay-apple.sh this does not
# need GitLab CI: it signs with the Developer ID in the login keychain and
# reads notarization credentials from .env.local.
#
# Usage (from the root of the source dir):
#   build/macos/dmg/clay-dist-local.sh <meson build dir> [bundle]
#
# .env.local (git-ignored) may define APPLE_ID, APPLE_TEAM_ID and
# APPLE_APP_PASS (an app-specific password). Without them, or with
# SKIP_NOTARIZE=1, the DMG is signed but not notarized.
# CODESIGN_IDENTITY overrides the identity.

set -euo pipefail

if [ ! -f build/macos/dmg/clay-dist-local.sh ]; then
  printf '\033[31m(ERROR)\033[0m: Call this script from the root of the source dir\n'
  exit 1
fi

BUILD_DIR="${1:-}"
APP="${2:-clay-$(uname -m).app}"
DIST_DIR="${DIST_DIR:-$PWD/../dist}"
BUNDLE_IDENTIFIER="xyz.kainzmayer.clay"
BUNDLE_NAME="Clay"
ENTITLEMENTS="$PWD/build/macos/dmg/clay-release.entitlements"

if [ -z "$BUILD_DIR" ] || [ ! -f "$BUILD_DIR/config.h" ]; then
  printf '\033[31m(ERROR)\033[0m: Pass the meson build dir (the one with config.h)\n'
  exit 1
fi
if [ ! -d "$APP/Contents" ]; then
  printf "\033[31m(ERROR)\033[0m: No bundle at $APP. Configure with -Ddmg=true -Drelocatable-bundle=yes and run 'meson install'\n"
  exit 1
fi

config_value()
{
  sed -n "s/^#define $1 \"\(.*\)\"/\1/p" "$BUILD_DIR/config.h"
}
CLAY_VERSION=$(config_value CLAY_VERSION)
CLAY_APP_VERSION=$(config_value CLAY_APP_VERSION)

if [ -n "${CODESIGN_IDENTITY:-}" ]; then
  SIGN_ID="$CODESIGN_IDENTITY"
else
  SIGN_ID=$(security find-identity -v -p codesigning | sed -n 's/.*"\(Developer ID Application:.*\)"/\1/p' | head -1)
fi
if [ -z "$SIGN_ID" ]; then
  printf '\033[31m(ERROR)\033[0m: No "Developer ID Application" identity in the keychain\n'
  exit 1
fi
printf "(INFO): Clay $CLAY_VERSION | bundle: $APP | identity: $SIGN_ID\n"


# 1. PREPARE BUNDLE
C="$APP/Contents"
## Not sealable: the bundle script's own .gitignore, debug symbols, dangling links
rm -f "$APP/.gitignore"
rm -rf "$C"/MacOS/*.dSYM
find "$C" -type l ! -exec test -e {} \; -delete
## Python's test suite has archives the notary service can not unpack
rm -rf "$C"/lib/Python.framework/Versions/*/lib/python*/test

## Info.plist (CFBundleExecutable can not be a symlink)
cp build/macos/Info.plist "$C/Info.plist"
python3 - "$C/Info.plist" "$BUILD_DIR/plug-ins/file_associations_mac.list" po/LINGUAS \
          "$BUNDLE_IDENTIFIER" "$BUNDLE_NAME" "$CLAY_VERSION" "$CLAY_APP_VERSION" <<'EOF'
import sys
plist, file_types, linguas, identifier, name, version, app_version = sys.argv[1:8]
text = open(plist).read()
text = text.replace("<string>clay-%CLAY_APP_VERSION%</string>", f"<string>clay-{app_version}</string>")
text = text.replace("%BUNDLE_IDENTIFIER%", identifier).replace("%BUNDLE_NAME%", name)
text = text.replace("%CLAY_VERSION%", version)
text = text.replace("%FILE_TYPES%", open(file_types).read().replace("\n", ""))
langs = [l.split()[0].replace("_", "-") for l in open(linguas) if l.strip() and not l.startswith("#")]
text = text.replace("%LOCALIZATIONS%", "".join(f"<string>{l}</string>" for l in dict.fromkeys(langs)))
open(plist, "w").write(text)
EOF
## The real minimum macOS is the highest one any bundled binary was built for
## (Homebrew builds for the OS it runs on), so macOS refuses to start it on older
## systems instead of letting it crash
MIN_MACOS=$(find "$C" -type f \( -name "*.dylib" -o -name "*.so" -o -path "*/MacOS/*" \) -exec otool -l {} \; 2>/dev/null \
            | awk '/LC_BUILD_VERSION/ {f=1} f && $1 == "minos" {print $2; f=0}' | sort -V | tail -1)
plutil -replace LSMinimumSystemVersion -string "${MIN_MACOS:-11.0}" "$C/Info.plist"
## Without the Liquid Glass Assets.car (actool failed) macOS must use AppIcon.icns
if [ ! -f "$C/Resources/Assets.car" ]; then
  plutil -remove CFBundleIconName "$C/Info.plist"
  printf '(INFO): no Assets.car, using AppIcon.icns\n'
fi
plutil -lint "$C/Info.plist" >/dev/null
printf "(INFO): minimum macOS: ${MIN_MACOS:-11.0}\n"


# 2. SIGN (inside-out: every Mach-O by header, then Python.framework, then the app)
printf '(INFO): signing\n'
python3 - "$APP" "$SIGN_ID" "$ENTITLEMENTS" <<'EOF'
import os, subprocess, sys
app, identity, entitlements = sys.argv[1:4]
magics = {b"\xfe\xed\xfa\xce", b"\xfe\xed\xfa\xcf", b"\xce\xfa\xed\xfe",
          b"\xcf\xfa\xed\xfe", b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca"}
def is_macho(path):
  with open(path, "rb") as f:
    return f.read(4) in magics
machos = [os.path.join(root, name)
          for root, _, names in os.walk(os.path.join(app, "Contents")) for name in names
          if not os.path.islink(os.path.join(root, name)) and is_macho(os.path.join(root, name))]
machos.sort(key=lambda p: p.count(os.sep), reverse=True)
sign = ["codesign", "--force", "--options", "runtime", "--timestamp", "-s", identity]
for path in machos:
  is_exe = "executable" in subprocess.run(["file", "-b", path], capture_output=True, text=True).stdout
  subprocess.run(sign + (["--entitlements", entitlements] if is_exe else []) + [path], check=True, capture_output=True)
framework_versions = os.path.join(app, "Contents/lib/Python.framework/Versions")
if os.path.isdir(framework_versions):
  for version in os.listdir(framework_versions):
    path = os.path.join(framework_versions, version)
    if not os.path.islink(path):
      subprocess.run(sign + [path], check=True, capture_output=True)
subprocess.run(sign + ["--entitlements", entitlements, app], check=True, capture_output=True)
print(f"(INFO): signed {len(machos)} Mach-O files")
EOF
codesign --verify --deep --strict "$APP"


# 3. CREATE DMG
mkdir -p "$DIST_DIR"
DMG="$DIST_DIR/Clay-$CLAY_VERSION-macOS-$(uname -m).dmg"
VOLNAME="$BUNDLE_NAME $CLAY_APP_VERSION"
STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT
mkdir "$STAGE/dmg"
ditto "$APP" "$STAGE/dmg/$BUNDLE_NAME.app"
ln -s /Applications "$STAGE/dmg/Applications"

## Background and icon layout (ported from 3_dist-clay-apple.sh, needs no Finder)
BG_PATH="$BUILD_DIR/clay-data/images/logo/clay-dmg.png"
if [ -f "$BG_PATH" ]; then
  cp "$BG_PATH" "$STAGE/dmg/.background.png"
  python3 -m venv "$STAGE/venv"
  "$STAGE/venv/bin/pip" install -q --disable-pip-version-check ds_store biplist mac_alias
  "$STAGE/venv/bin/python" - "$STAGE/dmg/.DS_Store" "$VOLNAME" "$BUNDLE_NAME.app" <<'EOF'
import sys
from datetime import datetime, timezone
import biplist, ds_store, mac_alias
ds_path, volname, app_name = sys.argv[1:4]
def create_alias(filename):
  volume = mac_alias.VolumeInfo(volname, datetime.now(timezone.utc),
                                fs_type=mac_alias.ALIAS_HFS_VOLUME_SIGNATURE, disk_type=mac_alias.ALIAS_FIXED_DISK,
                                attribute_flags=0, fs_id=bytes(2))
  target = mac_alias.TargetInfo(mac_alias.ALIAS_KIND_FILE, filename, folder_cnid=0, cnid=0,
                                creation_date=datetime.now(timezone.utc), creator_code=bytes(4), type_code=bytes(4),
                                folder_name=volname, carbon_path=f"{volname}:{filename}")
  return mac_alias.Alias(volume=volume, target=target)
store = ds_store.DSStore.open(ds_path, "w+")
store["."]["vSrn"] = ("long", 1)
store["."]["icvl"] = ("type", "icnv")
store["."]["icvp"] = {
  "backgroundType": 2, "backgroundColorRed": 1.0, "backgroundColorGreen": 1.0, "backgroundColorBlue": 1.0,
  "backgroundImageAlias": biplist.Data(create_alias(".background.png").to_bytes()),
  "viewOptionsVersion": 1, "gridSpacing": 100.0, "gridOffsetX": 0.0, "gridOffsetY": 0.0,
  "scrollPositionY": 0.0, "arrangeBy": "none", "labelOnBottom": True, "showItemInfo": False,
  "showIconPreview": False, "iconSize": 110.0, "textSize": 16.0,
}
store["."]["bwsp"] = {"WindowBounds": "{{100, 100}, {640, 512}}", "ShowToolbar": False,
                      "ShowPathbar": False, "ShowStatusBar": False}
store[app_name]["Iloc"] = (192, 282)
store["Applications"]["Iloc"] = (448, 282)
store[".background.png"]["Iloc"] = (524, 0)
store[".VolumeIcon.icns"]["Iloc"] = (524, 0)
store.flush()
store.close()
EOF
fi

## Read-write image first: the volume icon needs the custom-icon flag on the mounted root
rm -f "$DMG" "$STAGE/rw.dmg"
hdiutil create -volname "$VOLNAME" -srcfolder "$STAGE/dmg" -ov -format UDRW "$STAGE/rw.dmg" >/dev/null
VOLUME_ICON="$BUILD_DIR/build/macos/.VolumeIcon.icns"
if [ -f "$VOLUME_ICON" ]; then
  MOUNT=$(hdiutil attach -nobrowse -noautoopen "$STAGE/rw.dmg" | tail -1 | cut -f3-)
  cp "$VOLUME_ICON" "$MOUNT/.VolumeIcon.icns"
  SetFile -a C "$MOUNT"
  hdiutil detach "$MOUNT" >/dev/null
fi
hdiutil convert "$STAGE/rw.dmg" -format UDZO -o "$DMG" >/dev/null
codesign --force --timestamp -s "$SIGN_ID" "$DMG"
printf "(INFO): signed $DMG\n"


# 4. NOTARIZE AND STAPLE
if [ -n "${SKIP_NOTARIZE:-}" ]; then
  printf '(INFO): SKIP_NOTARIZE is set; DMG is signed but not notarized\n'
  exit 0
fi
if [ -f .env.local ]; then
  set -a; . ./.env.local; set +a
fi
APPLE_APP_PASS="${APPLE_APP_PASS:-${APPLE_APP_SPECIFIC_PASSWORD:-}}"
if [ -z "${APPLE_ID:-}" ] || [ -z "${APPLE_TEAM_ID:-}" ] || [ -z "$APPLE_APP_PASS" ]; then
  printf '\033[33m(WARNING)\033[0m: APPLE_ID, APPLE_TEAM_ID or APPLE_APP_PASS missing; DMG is signed but not notarized\n'
  exit 0
fi
printf '(INFO): notarizing (this takes a while)\n'
xcrun notarytool submit "$DMG" --apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" \
  --password "$APPLE_APP_PASS" --wait --output-format json > "$STAGE/notary.json"
STATUS=$(python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); print(d["status"], d["id"])' "$STAGE/notary.json")
printf "(INFO): notarization: $STATUS\n"
if [ "${STATUS%% *}" != 'Accepted' ]; then
  xcrun notarytool log "${STATUS##* }" --apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" --password "$APPLE_APP_PASS"
  exit 1
fi
xcrun stapler staple "$DMG"
spctl -a -t open --context context:primary-signature -vv "$DMG"
printf "(INFO): done: $DMG\n"
