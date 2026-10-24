---
name: clay-mcp
description: >-
  Drive Clay Designer via the AI Agent Bridge (MCP). Use when opening images in
  Clay, scaling/exporting, content-aware fill, GEGL, PDB, execute_python, MCP
  autostart, or troubleshooting tools/clay-mcp/server.py on 127.0.0.1:9877.
---

# Clay MCP

Clay is the image editor in this repo (`T:\Apps\clay\_install\bin\clay.exe`). Agents talk JSON-RPC to a live Clay process.

## Start Clay + bridge

1. Launch `_install\bin\clay.exe` (or `tools/clay-mcp/start-clay.ps1`).
2. Autostart: **Tools → ✓ Autostart MCP Server on Launch** (checkmark only when on). Saved in `%APPDATA%\Clay\3.36\mcp-bridge.json`.
3. If the port is dead: **Tools → AI Agent Bridge (MCP Server)…**
4. Cursor MCP server: `.cursor/mcp.json` → `tools/clay-mcp/server.py` (`CLAY_MCP_HOST` / `CLAY_MCP_PORT`, default `127.0.0.1:9877`).

Python plug-ins need MSYS clang64 + `GI_TYPELIB_PATH`. If **Filters → VFX** is missing, run `tools/clay-mcp/configure-windows-python.ps1` then start Clay from `start-clay.ps1`.

## Bridge MCP tools (14)

Defined in `tools/clay-mcp/server.py` (`TOOLS`), each maps to a TCP method on the bridge:

| MCP tool | Bridge method | What it does |
|----------|---------------|----------------|
| `open_image_in_clay` | `open_image` | Open a file path in Clay |
| `get_session_state` | `get_session_state` | Open images, sizes, last log lines |
| `get_canvas_preview` | `get_canvas_snapshot` | PNG snapshot (duplicate, does not wipe undo) |
| `beautify_image` | `beautify_image` | Presets: `auto_beautify`, `skin_smooth_portrait`, `cinematic_grade`, `hdr_pop`, `denoise_and_sharpen` |
| `content_aware_fill` | `content_aware_fill` | Heal current selection (optional `rect` / `make_random_white_patch`). Tries Resynthesizer PDB if present, else texture copy |
| `resize_image` | `resize_image` | Scale (`width`/`height` or `scale_percent`); optional `dpi` |
| `apply_gegl_filter` | `apply_gegl_operation` | One GEGL op + properties |
| `edit_layer_or_selection` | `manage_layers_and_selections` | `select_rectangle` + `rect`, or `none` |
| `add_text_overlay` | `execute_python` | New text layer |
| `export_image` | `export_image` | Save to path |
| `search_clay_app_store` | `store_search` | Keyword search in store catalog |
| `install_clay_plugin` | `store_search` | Looks up AppStream id (does not download GEX yet) |
| `run_pdb_procedure` | `call_pdb` | Any PDB procedure name + arguments |
| `execute_clay_python` | `execute_python` | Python in Clay with `Clay`, `Gegl`, `image`, `drawable` |

## Resynthesizer vs Inhaltssensitives Füllen

**Searching the store for “resynthesizer” installs Inhaltssensitives Füllen**, not bootchk Resynthesizer. The catalog tags that DLC with the word `resynthesizer`; the code is GEGL diffusion (`mean-curvature-blur` when `gegl:inpaint` is missing).

| What you see | What it is |
|--------------|------------|
| **Filters → Enhance → Content-Aware Fill** | `plug-in-inpaint-heal` — DLC `xyz.kainzmayer.clay.extension.inpaint-heal` |
| **Filters → Enhance → Heal Transparency** | `plug-in-heal-transparency` — **second procedure in the same .py**, not a second plugin |
| bootchk Heal Selection / Uncrop / Map Style | **Not installed.** PDB has no `python-fu-heal-selection` / `plug-in-resynthesizer` |

Files: `extensions/inpaint-heal/inpaint-heal.py`. After rebuild: `_install/lib/clay/3.0/extensions/xyz.kainzmayer.clay.extension.inpaint-heal/`.

## Work rules

- Keep the **AI Agent Bridge** process alive (autostart daemon or dialog). Do not pixel-loop on the main thread.
- **Scale:** `Clay.InterpolationType.LOHALO` for downscale, `NOHALO` for upscale. Read `context_get_interpolation()` first (user may have set Cubic). Do not change DPI unless asked.
- **JPEG export:** `file-jpeg-export` quality `1.0`, `sub-sampling-1x1`, smoothing `0`. Status `3` = `SUCCESS`.
- **Undo:** use `edit_fill` / copy-paste / filters / layer mask, never `set_pixel` (not undoable → empty history). Snapshot via `image.duplicate()` then save the copy. MCP runs inside `clay-mcp-bridge.py`, so the History list often shows that plug-in name (GIMP/Clay PDB has no separate “agent undo” API). Prefer several small undo groups over one giant `undo_group`.
- Content-aware fill is **not** Photoshop Content-Aware; Resynthesizer is optional (`tools/clay-mcp/INSTALL-RESYNTHESIZER.md`).
- AppStream / Apple bundle ID: `xyz.kainzmayer.clay` (later `org.niiii.clay`). Extensions: `xyz.kainzmayer.clay.extension.*`.

## Paths

- Prefix: `T:\Apps\clay\_install`
- Bridge: `extensions/clay-mcp-bridge/clay-mcp-bridge.py` (copy into `_install` after edits; restart Clay)
- Logos: `tools/clay-mcp/clay-logo.jpg` and `clay-logo-neon-*.jpg`
