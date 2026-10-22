#!/bin/bash

# Turn the bundle made by 2_bundle-gimp-uni_base.py (meson -Ddmg=true
# -Drelocatable-bundle=yes, then 'meson install') into a signed, notarized
# Clay DMG on a developer Mac. Unlike 3_dist-gimp-apple.sh this does not
# need GitLab CI: it signs with the Developer ID in the login keychain and
# reads notarization credentials from .env.local.
#
# Usage (from the root of the source dir):
#   build/macos/dmg/clay-dist-local.sh <meson build dir> [bundle]
#
# .env.local (git-ignored) may define APPLE_ID, APPLE_TEAM_ID and
# APPLE_APP_PASS (an app-specific password). Without them the DMG is
# signed but not notarized. CODESIGN_IDENTITY overrides the identity.

set -euo pipefail

if [ ! -f build/macos/dmg/clay-dist-local.sh ]; then
  printf '\033[31m(ERROR)\033[0m: Call this script from the root of the source dir\n'
  exit 1
fi

BUILD_DIR="${1:-}"
APP="${2:-gimp-$(uname -m).app}"
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
GIMP_VERSION=$(config_value GIMP_VERSION)
GIMP_APP_VERSION=$(config_value GIMP_APP_VERSION)

if [ -n "${CODESIGN_IDENTITY:-}" ]; then
  SIGN_ID="$CODESIGN_IDENTITY"
else
  SIGN_ID=$(security find-identity -v -p codesigning | sed -n 's/.*"\(Developer ID Application:.*\)"/\1/p' | head -1)
fi
if [ -z "$SIGN_ID" ]; then
  printf '\033[31m(ERROR)\033[0m: No "Developer ID Application" identity in the keychain\n'
  exit 1
fi
printf "(INFO): Clay $GIMP_VERSION | bundle: $APP | identity: $SIGN_ID\n"


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
          "$BUNDLE_IDENTIFIER" "$BUNDLE_NAME" "$GIMP_VERSION" "$GIMP_APP_VERSION" <<'EOF'
import sys
plist, file_types, linguas, identifier, name, version, app_version = sys.argv[1:8]
text = open(plist).read()
text = text.replace("<string>gimp-%GIMP_APP_VERSION%</string>", f"<string>clay-{app_version}</string>")
text = text.replace("%BUNDLE_IDENTIFIER%", identifier).replace("%BUNDLE_NAME%", name)
text = text.replace("%GIMP_VERSION%", version)
text = text.replace("%FILE_TYPES%", open(file_types).read().replace("\n", ""))
langs = [l.split()[0].replace("_", "-") for l in open(linguas) if l.strip() and not l.startswith("#")]
text = text.replace("%LOCALIZATIONS%", "".join(f"<string>{l}</string>" for l in dict.fromkeys(langs)))
open(plist, "w").write(text)
EOF
plutil -lint "$C/Info.plist" >/dev/null


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
DMG="$DIST_DIR/Clay-$GIMP_VERSION-macOS-$(uname -m).dmg"
STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT
ditto "$APP" "$STAGE/$BUNDLE_NAME.app"
ln -s /Applications "$STAGE/Applications"
rm -f "$DMG"
hdiutil create -volname "$BUNDLE_NAME $GIMP_APP_VERSION" -srcfolder "$STAGE" -ov -format UDZO "$DMG" >/dev/null
codesign --force --timestamp -s "$SIGN_ID" "$DMG"
printf "(INFO): signed $DMG\n"


# 4. NOTARIZE AND STAPLE
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
