# macOS agent: commit the missing themes and icons

Read this file, do the work, commit, and push to `origin/master`. Windows will pull that commit and rebuild. No other coordination is required.

## What is wrong

Windows and yesterday's published DMG (`Clay-3.36.0-macOS-arm64.dmg`, tag `v3.36.0`, commit `cb23e68`) were built from the same git tree. That tree only has:

- GTK themes: `Clay`, `Default`, `System`
- Icon themes: `Default`, `Legacy`, `hicolor`

The new themes (about 20, including Neon) and the new icons were created on this Mac and never committed. They are not on GitHub. A clean build cannot show them.

## Where they are on this Mac

Search in this order and take the newest complete copy of each name:

1. Uncommitted files in this source tree. Run `git status`. Anything under `themes/` or `gimp-data/icons/` that is not `Clay`, `Default`, `System`, `Legacy`, or `hicolor` belongs in the commit.
2. Local app bundles. They are gitignored (`/clay-*.app/`), so git will not show them.
   - `clay-*.app/Contents/Resources/share/clay/3.0/themes`
   - `clay-*.app/Contents/Resources/share/clay/3.0/icons`
   - Also check `share/gimp/3.0/themes` and `share/gimp/3.0/icons` in an older bundle.
3. The user profile.
   - `~/Library/Application Support/Clay/3.36/themes`
   - `~/Library/Application Support/Clay/3.36/icons`
4. A local install prefix next to the source tree, such as `_install/share/clay/3.0/themes` and `_install/share/clay/3.0/icons`.

A GTK theme folder contains `gimp-dark.css` and usually `gimp-light.css`. An icon theme folder contains `index.theme`. List every directory you find and copy the ones whose names are not already in git.

## Put GTK themes in the source tree

For each new theme named `<Name>`:

- Copy the folder to `themes/<Name>/`.
- Add `themes/<Name>/meson.build` using `themes/Clay/meson.build` as the pattern. Install the CSS files that actually exist into `gimpdatadir / 'themes' / '<Name>'`.
- If the folder has `ui/`, add `themes/<Name>/ui/meson.build` the same way `themes/Clay/ui/meson.build` does, with that theme's name in `install_dir`.
- Add `subdir('<Name>')` in `themes/meson.build` next to `Default`, `Clay`, and `System`.

Leave `Clay`, `Default`, and `System` as they are.

## Put icon themes in the source tree

For each new icon theme named `<Name>`:

- The folder must contain `index.theme`. Clay skips an icon theme that has no `index.theme` (`app/gui/icon-themes.c`).
- Copy the folder to `gimp-data/icons/<Name>/`.
- Do not add `<Name>` to the `themes = ['Default', 'Legacy', 'hicolor']` list in `gimp-data/icons/meson.build`. That loop expects the Default/Legacy layout and its icon lists.
- After that loop, add `subdir('<Name>')`.
- Give `<Name>` its own `meson.build` that installs `index.theme` and the image files, keeping the directory layout (`scalable/`, `16x16/`, and so on). `install_subdir()` is the right tool when there are many files. Install to `gimpiconsdir / '<Name>'`.

Leave `Default`, `Legacy`, and `hicolor` as they are.

## Commit and push

- Do not commit `clay-*.app/`, `_build/`, `_install/`, `_src/`, `.env`, or signing keys.
- Delete this `MACOS-AGENT.md` file in the same commit. It is only the instruction.
- Commit on `master` with a message that names the themes and icon themes you added.
- Push to `origin`:

```bash
git push origin master
```

Windows then runs `git pull` and rebuilds. After that install, Preferences must list the new theme names and the new icon-theme names.

## Done when

`git ls-files themes gimp-data/icons` on GitHub shows every theme and icon theme that the Mac app was showing, each wired into meson as above, and `MACOS-AGENT.md` is gone.
