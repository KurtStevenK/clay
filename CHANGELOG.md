# Changelog

All notable changes to **Clay** are documented in this file.

Clay 3.36.0 is the first branded release of the Clay fork. It is based on the
GNU Image Manipulation Program (GIMP) 3.36 development line, with Clay-specific
branding, packaging, themes, and a renamed but compatible plug-in API (`libclay`).

## [Unreleased]

### Website

- Remove the donate page (`/donate`), its SumUp section, and the Donate links
  in the site navigation.

### Welcome dialog

- Remove the "Donating" welcome link and the "Other donation options" link
  that pointed at the deleted donate page. The Liberapay link stays.

## [3.36.0] — 2026-10-23

### Summary

- **Clay Design** ships Clay as *Clay Image Beautification Program* under the
  application ID `xyz.kainzmayer.ClayDesign`, with install paths, man pages, and
  user-visible strings renamed from GIMP to Clay while keeping the GIMP plug-in
  ABI surface (`Clay-3.0` / `clay-3.0`, PDB procedure names, Script-Fu
  interpreter compatibility links).
- **Windows** is the primary target for this release (`clay-3.36`); **macOS**
  release automation (DMG, signing workflow) and **Linux** builds are supported
  in-tree.
- Upstream GIMP 3.36 fixes and features from the merged development branch
  remain in the codebase (font loading, heal tool i18n, display fixes, metadata,
  translations, and more).

### Branding and user-visible rename

- Publish Clay under `xyz.kainzmayer.ClayDesign` and ClayDesign copyright
  (`c6df4a59c6b`).
- Clay GTK theme, splash artwork, branded `INSTALL`, and project website pages
  (`b82d7c211d9`, `869af68c77d`).
- User-facing rename: single app ID, `clay` install folders, links to Clay
  (`2b3f706ea23`).
- Man pages installed as `clay`, `clay-console`, and `claytool`
  (`fd892f1140e`).
- Tips, crash dialog, and related strings say Clay (`7b3834f04f6`).
- Windows installer uses Clay names, folders, and shortcuts (`141c5f7d820`).
- `NOTICE` documents the GIMP fork and the retained plug-in interface
  (`cf17eb52876`).
- Large tree rename: `gimp-data` → `clay-data`, `pygimp` → `pyclay`, remaining
  GIMP identifiers in source, plus committed GTK themes and icon themes (Neon,
  Galaxy, Legacy, Color, and others) (`92cb8843ffb`, `e73beb400c8`).

### Plug-in API and libraries

- Libraries and introspection renamed to **libclay** / **Clay-3.0**; macOS
  in-build typelibs fixed after the rename (`e9f050265de`).
- `gimp-script-fu-interpreter-3.0` installed as a compatibility symlink on
  macOS and Linux (`119a4d5c79d`).

### Themes and icons

- macOS checkout documents committing themes and icons (`e73beb400c8`).
- GTK3: fix Clay theme selection rule rejected by GTK3 (`9e3a203b1a3`).
- **Galaxy** and **Neon** toolbox icon animations use stepped colour cycles
  instead of per-frame linear interpolation, reducing idle CPU use when the
  toolbox is open (this release).
- **Legacy** icon theme inherits **hicolor** so missing status icons resolve
  correctly (this release).
- Add **image-missing** and **image-missing-symbolic** status icons to hicolor
  (this release).

### Build, CI, and release

- Dependabot enabled for GitHub Actions (`06d7a5ee555`).
- Bump `actions/checkout` and `actions/setup-node` to v7 (`f37d775ad07`,
  `9c533881668`).
- VirusTotal workflow and secret-hygiene documentation (`b82d7c211d9`; see
  `SECURITY.md`).
- **macOS**: Clay app icon from Clay logo (`3538ffc6f55`); bundle script fixes
  for Clay and Homebrew (`3436f6eb87f`); gexiv2 0.16 on Homebrew (`7bca87d79ab`);
  ignore generated `.app` in source tree (`f514049396f`); local signed/notarized
  DMG script (`487fd3c21c3`); DMG/volume/file branding (`4ff94f8ccd0`,
  `613e5694e91`); bundle naming `clay-<arch>.app` and SDK test fix
  (`c051f530ff4`); **Build & Release** workflow for macOS DMG (`437647f9ddc`);
  `actool` fallback to `AppIcon.icns` (`00e7e21cea1`); optional SDL3/libomp
  bundling (`9fe39013bc3`); Info.plist copyright and creator code (`55a3b4a322b`).
- **Build & Release**: manual workflow runs on `v*` tags as well as tag push
  (`37dcc4fed23`, PR #8).
- **Windows**: Meson resource build copies configured `.rc` files after
  `git-version.h` is generated, fixing windres races (GNOME/gimp#6257) (this
  release).

### Housekeeping

- SumUp donate button label fix; ignore Cursor plan files (`3a40f727523`).

### Contributors

Clay 3.36.0 reflects work by **Kurt Steven Kainzmayer** and collaborators,
including automation and rename commits on `KurtStevenK/clay`, plus upstream
GIMP authors whose copyright notices remain in individual source files.

### Commit reference (Clay fork, chronological)

| Commit     | Description |
| ---------- | ----------- |
| `c6df4a59` | Publish Clay under xyz.kainzmayer.ClayDesign |
| `7e64e283` | Show the copyright as ClayDesign Contributors |
| `b82d7c21` | Clay theme, splash, VirusTotal workflow, secret hygiene |
| `869af68c` | Clay site pages and branded INSTALL |
| `06d7a5ee` | Enable Dependabot version updates |
| `9c533881` | build(deps): bump actions/setup-node to 7 |
| `f37d775a` | build(deps): bump actions/checkout to 7 |
| `3a40f727` | Fix SumUp donate button; ignore Cursor plan files |
| `3538ffc6` | Clay logo for macOS app icon |
| `7bca87d7` | Build with gexiv2 0.16 on macOS Homebrew |
| `3436f6eb` | Fix macOS .app bundle script for Clay and Homebrew |
| `9e3a203b` | Fix Clay theme selection rule (GTK3) |
| `f5140493` | Ignore macOS .app bundle in source tree |
| `487fd3c2` | Local macOS release script (signed, notarized DMG) |
| `e9f05026` | Fix in-build typelibs on macOS for libclay* rename |
| `4ff94f8c` | Clay logo for DMG, volume, and file icons |
| `c051f530` | macOS bundle: SDK test, `clay-<arch>.app` naming |
| `613e5694` | clay-dist-local.sh: styled DMG, volume icon, min macOS |
| `437647f9` | Build & Release workflow for macOS DMG |
| `00e7e21c` | macOS CI: fallback when actool fails |
| `9fe39013` | Bundle SDL3 and libomp only when present |
| `2b3f706e` | App ID and clay install folders |
| `fd892f11` | Man pages: clay, clay-console, claytool |
| `7b3834f0` | Clay in tips and crash dialog |
| `141c5f7d` | Windows installer: Clay names and folders |
| `119a4d5c` | Script-Fu interpreter compatibility link |
| `55a3b4a3` | macOS Info.plist copyright and creator code |
| `cf17eb52` | NOTICE: GIMP fork and plug-in interface |
| `37dcc4fe` | Build & Release on manual v* tag runs |
| `e73beb40` | macOS: commit themes and icons in checkout |
| `92cb8843` | Remaining GIMP→Clay rename; themes and icons |

[3.36.0]: https://github.com/KurtStevenK/clay/releases/tag/v3.36.0
