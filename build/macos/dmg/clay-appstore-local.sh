#!/bin/bash

# Turn the bundle signed by clay-dist-local.sh (run it first, SKIP_NOTARIZE=1
# is enough) into a sandboxed Mac App Store package and upload it to App
# Store Connect, where it shows up in TestFlight.
#
# Usage (from the root of the source dir):
#   build/macos/dmg/clay-appstore-local.sh [bundle]
#
# Needs in the login keychain: "Apple Distribution" and "3rd Party Mac
# Developer Installer" (Mac Installer Distribution) identities.
# PROVISIONING_PROFILE: a Mac App Store profile for the bundle identifier.
# ASC_KEY_ID, ASC_ISSUER_ID: App Store Connect API key; altool reads the key
# from ~/.appstoreconnect/private_keys/AuthKey_<ASC_KEY_ID>.p8.
# BUILD_NUMBER: CFBundleVersion, must grow with each upload (default: date).
# SKIP_UPLOAD=1 only builds and validates the package.

set -euo pipefail

if [ ! -f build/macos/dmg/clay-appstore-local.sh ]; then
  printf '\033[31m(ERROR)\033[0m: Call this script from the root of the source dir\n'
  exit 1
fi

SRC_APP="${1:-clay-$(uname -m).app}"
DIST_DIR="${DIST_DIR:-$PWD/../dist}"
BUNDLE_IDENTIFIER="xyz.kainzmayer.clay"
BUNDLE_NAME="Clay"
TEAM_ID="${APPLE_TEAM_ID:-XPPUZJDN56}"
BUILD_NUMBER="${BUILD_NUMBER:-$(date +%Y%m%d%H%M)}"
ENTITLEMENTS_DIR="$PWD/build/macos/dmg"

if [ -f .env.local ]; then
  set -a; . ./.env.local; set +a
fi
if [ ! -f "${PROVISIONING_PROFILE:-}" ]; then
  printf '\033[31m(ERROR)\033[0m: Set PROVISIONING_PROFILE to a Mac App Store profile\n'
  exit 1
fi
SIGN_ID=$(security find-identity -v -p codesigning | sed -n "s/.*\"\(Apple Distribution:.*($TEAM_ID)\)\"/\1/p" | head -1)
INSTALLER_ID=$(security find-identity -v | sed -n "s/.*\"\(3rd Party Mac Developer Installer:.*($TEAM_ID)\)\"/\1/p" | head -1)
if [ -z "$SIGN_ID" ] || [ -z "$INSTALLER_ID" ]; then
  printf '\033[31m(ERROR)\033[0m: Need "Apple Distribution" and "3rd Party Mac Developer Installer" identities\n'
  exit 1
fi

STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT
APP="$STAGE/$BUNDLE_NAME.app"
ditto "$SRC_APP" "$APP"
C="$APP/Contents"
VERSION=$(plutil -extract CFBundleShortVersionString raw "$C/Info.plist")
printf "(INFO): Clay $VERSION ($BUILD_NUMBER) | identity: $SIGN_ID\n"


# 1. PREPARE BUNDLE
## App Store validation rejects the com.apple.xcode.dsym bundles next to the plug-ins
find "$C" -name "*.dSYM" -prune -exec rm -rf {} +
## App Store processing wants the standard framework layout (Current, Python,
## Resources links); the headers are only needed to build against Python
PY_FW="$C/lib/Python.framework"
if [ -d "$PY_FW/Versions" ]; then
  PY_VERSION=$(ls "$PY_FW/Versions" | grep -v Current | head -1)
  rm -rf "$PY_FW/Versions/$PY_VERSION/Headers" "$PY_FW/Versions/$PY_VERSION/include"
  ln -sfn "$PY_VERSION" "$PY_FW/Versions/Current"
  ln -sfn Versions/Current/Python "$PY_FW/Python"
  ln -sfn Versions/Current/Resources "$PY_FW/Resources"
fi
plutil -replace CFBundleVersion -string "$BUILD_NUMBER" "$C/Info.plist"
plutil -replace ITSAppUsesNonExemptEncryption -bool NO "$C/Info.plist"
cp "$PROVISIONING_PROFILE" "$C/embedded.provisionprofile"
sed -e "s/%TEAM_ID%/$TEAM_ID/g" -e "s/%BUNDLE_IDENTIFIER%/$BUNDLE_IDENTIFIER/g" \
    "$ENTITLEMENTS_DIR/clay-appstore.entitlements" > "$STAGE/app.entitlements"


# 2. SIGN (inside-out; helper executables inherit the app's sandbox)
printf '(INFO): signing\n'
python3 - "$APP" "$SIGN_ID" "$STAGE/app.entitlements" "$ENTITLEMENTS_DIR/clay-appstore-helper.entitlements" <<'PY'
import os, plistlib, subprocess, sys
app, identity, app_entitlements, helper_entitlements = sys.argv[1:5]
main_exe = os.path.join(app, "Contents/MacOS",
                        plistlib.load(open(os.path.join(app, "Contents/Info.plist"), "rb"))["CFBundleExecutable"])
magics = {b"\xfe\xed\xfa\xce", b"\xfe\xed\xfa\xcf", b"\xce\xfa\xed\xfe",
          b"\xcf\xfa\xed\xfe", b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca"}
def is_macho(path):
  with open(path, "rb") as f:
    return f.read(4) in magics
machos = [os.path.join(root, name)
          for root, _, names in os.walk(os.path.join(app, "Contents")) for name in names
          if not os.path.islink(os.path.join(root, name)) and is_macho(os.path.join(root, name))]
machos.sort(key=lambda p: p.count(os.sep), reverse=True)
sign = ["codesign", "--force", "--timestamp", "-s", identity]
helpers = 0
for path in machos:
  if path == main_exe:
    continue
  is_exe = "executable" in subprocess.run(["file", "-b", path], capture_output=True, text=True).stdout
  helpers += is_exe
  subprocess.run(sign + (["--entitlements", helper_entitlements] if is_exe else []) + [path], check=True, capture_output=True)
framework_versions = os.path.join(app, "Contents/lib/Python.framework/Versions")
if os.path.isdir(framework_versions):
  for version in os.listdir(framework_versions):
    path = os.path.join(framework_versions, version)
    if not os.path.islink(path):
      subprocess.run(sign + [path], check=True, capture_output=True)
subprocess.run(sign + ["--entitlements", app_entitlements, app], check=True, capture_output=True)
print(f"(INFO): signed {len(machos)} Mach-O files, {helpers} helper executables")
PY
codesign --verify --deep --strict "$APP"


# 3. PACKAGE
mkdir -p "$DIST_DIR"
PKG="$DIST_DIR/Clay-$VERSION-$BUILD_NUMBER-AppStore.pkg"
productbuild --component "$APP" /Applications --sign "$INSTALLER_ID" "$PKG" >/dev/null
printf "(INFO): $PKG\n"


# 4. VALIDATE AND UPLOAD
ALTOOL_AUTH=(--apiKey "$ASC_KEY_ID" --apiIssuer "$ASC_ISSUER_ID")
xcrun altool --validate-app --type macos -f "$PKG" "${ALTOOL_AUTH[@]}"
if [ -n "${SKIP_UPLOAD:-}" ]; then
  printf '(INFO): SKIP_UPLOAD is set; package validated, not uploaded\n'
  exit 0
fi
xcrun altool --upload-app --type macos -f "$PKG" "${ALTOOL_AUTH[@]}"
printf '(INFO): uploaded; TestFlight lists the build once App Store Connect has processed it\n'
