# Clay Designer MCP

Connect an AI agent (Cursor, Claude Desktop, …) to a running Clay window the same way Blender MCP drives Blender.

## Setup

1. Build and run Clay (`meson install` → `_install\bin\clay.exe`).
2. **Windows:** Python extensions need MSYS clang64 + typelibs. Run once:
   `powershell -ExecutionPolicy Bypass -File tools/clay-mcp/configure-windows-python.ps1`
   Start Clay with:
   `powershell -ExecutionPolicy Bypass -File tools/clay-mcp/start-clay.ps1`
3. Open an image, then choose **Tools → AI Agent Bridge (MCP Server)…** and leave the dialog open. Clay listens on `127.0.0.1:9877`.
4. Point Cursor at this server. A workspace file is already at `.cursor/mcp.json`.

Agent skill: `.cursor/skills/clay-mcp/SKILL.md`.

The MCP process talks stdio to the agent and TCP JSON-RPC to Clay.

## Tools

| Tool | What it does |
| --- | --- |
| `open_image_in_clay` | Load a file into Clay and show it |
| `get_canvas_preview` | Write a PNG snapshot and return its path |
| `beautify_image` | Run Auto Beautify / portrait / cinematic / HDR presets |
| `content_aware_fill` | Inhaltssensitives Füllen (optional `rect`) |
| `apply_gegl_filter` | Any GEGL operation on the active layer |
| `export_image` | Save the active image |
| `execute_clay_python` | Run Python with `Clay`, `Gegl`, `image`, `drawable` |

Environment overrides: `CLAY_MCP_HOST`, `CLAY_MCP_PORT`.
