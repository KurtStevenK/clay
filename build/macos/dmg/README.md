# GIMP Apple HowTo

If you are a macOS package maintainer, check the documentation at:
<https://testing.developer.gimp.org/core/maintainer/accounts/appledev/>

## Clay release DMG on a developer Mac

`3_dist-gimp-apple.sh` only signs and notarizes on GitLab CI. To make a
signed, notarized Clay DMG locally with Homebrew:

1. Build babl, GEGL and Clay with `build/macos/1_build-deps-macports.sh` and
   `build/macos/2_build-gimp-macports.sh` (both use Homebrew when MacPorts
   is not installed).
2. Reconfigure the Clay build dir for a bundle and install it, which runs
   `2_bundle-gimp-uni_base.py` and writes `clay-<arch>.app` here:

   ```sh
   meson configure <build dir> -Drelocatable-bundle=yes -Ddmg=true
   meson install -C <build dir>
   ```

3. Put the notarization credentials in `.env.local` (git-ignored):

   ```sh
   APPLE_ID=<Apple ID email>
   APPLE_TEAM_ID=<Team ID>
   APPLE_APP_PASS=<app-specific password from appleid.apple.com>
   ```

4. Sign, notarize and staple:

   ```sh
   build/macos/dmg/clay-dist-local.sh <build dir>
   ```

   The DMG lands in `../dist/`. It signs with the first
   "Developer ID Application" identity in the keychain unless
   `CODESIGN_IDENTITY` is set, using `clay-release.entitlements`.
   `SKIP_NOTARIZE=1` skips notarization for a quick test.

## GitHub Actions

`.github/workflows/build-release.yml` ("Build & Release") runs the same steps on a
`macos-15` runner. A `v*` tag builds, notarizes and attaches the DMG to the GitHub
release, which then triggers the VirusTotal scan. Pull requests that touch the
macOS build only build and sign.

The DMG runs on the runner's macOS version or newer, because Homebrew builds its
libraries for the OS it runs on. A DMG built locally on a newer macOS needs that
newer macOS.

Upload the signing secrets once with `scripts/push-apple-signing-to-github.sh`
(`CSC_LINK`, `CSC_KEY_PASSWORD`, `APPLE_ID`, `APPLE_APP_SPECIFIC_PASSWORD`,
`APPLE_TEAM_ID`).
