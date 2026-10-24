# Resynthesizer for Clay (GIMP 3 API)

Clay’s built-in **Inhaltssensitives Füllen** uses GEGL diffusion when `gegl:inpaint` is missing. For Photoshop-like healing, install [bootchk/resynthesizer](https://github.com/bootchk/resynthesizer) (GIMP 3 branch `resynthesizer3`).

## Windows

Upstream does **not** ship Windows binaries. Build from source in **MSYS clang64** with Clay’s prefix:

```bash
pacman -S mingw-w64-clang-x86_64-gimp3-devel meson ninja git
cd /t/Apps/clay/third_party
git clone -b resynthesizer3 --depth 1 https://github.com/bootchk/resynthesizer.git
cd resynthesizer
meson setup _build --prefix=/t/Apps/clay/_install
meson compile -C _build
meson install -C _build
```

The engine compiles, but **linking against Clay fails**: Resynthesizer calls `gimp_*` while Clay’s `gimp-3.0.pc` exports `clay_*` (`gimp_main` vs `clay_main`, etc.). The repo is cloned at `third_party/resynthesizer`; a Clay port needs symbol mapping or `#define gimp_main clay_main` (and the rest of libgimp) before `meson install` will produce a working `plug-in-resynthesizer`.

Until that lands, `content_aware_fill` uses texture copy / `plug-in-inpaint-heal`.

Restart Clay. MCP `content_aware_fill` will call Resynthesizer automatically when a procedure is registered.

## Verify

In Clay: **Filters → Enhance** — look for Resynthesizer / Heal selection entries, or run MCP and check `resynthesizer_installed: true` in the JSON result.
