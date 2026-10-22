#!/usr/bin/env bash
# Upload the macOS signing and notarization secrets used by
# .github/workflows/build-release.yml. Never commit .p12 files or .env.local.
#
# Reads APPLE_ID, APPLE_TEAM_ID and APPLE_APP_PASS from .env.local (see
# build/macos/dmg/README.md) and asks only for the .p12 password.
set -euo pipefail

REPO="KurtStevenK/clay"
P12_PATH="${APPLE_P12_FILE:-${HOME}/private/apple-developer-id/DeveloperIDApplication-for-ci.p12}"

cd "$(dirname "$0")/.."
if [[ ! -f "$P12_PATH" ]]; then
  echo "No .p12 at $P12_PATH." >&2
  echo "Export it from Keychain Access (My Certificates > Developer ID Application > Export) or set APPLE_P12_FILE." >&2
  exit 1
fi
if [[ ! -f .env.local ]]; then
  echo "No .env.local with APPLE_ID, APPLE_TEAM_ID and APPLE_APP_PASS." >&2
  exit 1
fi
set -a; . ./.env.local; set +a
for name in APPLE_ID APPLE_TEAM_ID APPLE_APP_PASS; do
  if [[ -z "${!name:-}" ]]; then
    echo "$name is missing in .env.local." >&2
    exit 1
  fi
done

read -rsp "Password of $(basename "$P12_PATH"): " P12_PASS
echo ""

echo "Uploading secrets to ${REPO}..."
base64 < "$P12_PATH" | tr -d '\n' | gh secret set CSC_LINK --repo "$REPO"
printf '%s' "$P12_PASS" | gh secret set CSC_KEY_PASSWORD --repo "$REPO"
printf '%s' "$APPLE_ID" | gh secret set APPLE_ID --repo "$REPO"
printf '%s' "$APPLE_APP_PASS" | gh secret set APPLE_APP_SPECIFIC_PASSWORD --repo "$REPO"
printf '%s' "$APPLE_TEAM_ID" | gh secret set APPLE_TEAM_ID --repo "$REPO"
echo "Done. Push a v* tag to build, notarize and publish the macOS DMG."
