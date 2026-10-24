# Clay

**Clay Image Beautification Program**

Maintained by [xyz.kainzmayer.ClayDesign](https://claydesign.web.app).

Clay is a free image editor for Windows, Linux, and macOS. Mobile editions for iOS, Android, Huawei, and Microsoft are planned. The current release is 3.36 for Windows.

© 2026 ClayDesign Contributors.

## License

Clay is [GPL-3.0-or-later](COPYING). `libclay` is [LGPL-3.0-or-later](libclay/COPYING). Component and data licensing notes: [LICENSE](LICENSE); bundled assets: [clay-data/LICENSE](clay-data/LICENSE).

Fork of [GIMP](https://www.gimp.org/) (the GNU Image Manipulation Program), not affiliated with the GIMP project. Upstream trademarks and attribution: [NOTICE](NOTICE). Copyright belongs to the respective contributors; Clay Design maintains this fork. Questions: [open an issue](https://github.com/KurtStevenK/clay/issues).

The plug-in interface (`libclay`, clay-* PDB, `Clay-3.0`, `clay-3.0` pkg-config) stays compatible so scripts and ports keep working while the app is named Clay.

## Verify downloads

Every GitHub release is scanned on [VirusTotal](https://www.virustotal.com). The release notes list each installer's detection count and link to its full multi-engine report. Heuristic flags on large native GTK apps are common, so read the whole report rather than a single detection. See [SECURITY.md](SECURITY.md).

## App Store and AI agent (MCP)

Clay’s **Extensions → Clay App Store (DLC Shop)** dialog browses curated plugins (Content-Aware Fill / *Inhaltssensitives Füllen*, Beautify, VFX, batch export, and the MCP bridge). Search is live and bilingual.

To let an AI agent drive Clay like Blender MCP:

1. Start Clay, open an image, then **Tools → AI Agent Bridge (MCP Server)…**
2. Use the MCP server in [tools/clay-mcp](tools/clay-mcp) (see `.cursor/mcp.json`).

## Building

Desktop builds use the Meson files already in this tree: [build/windows](build/windows), [build/linux](build/linux), and [build/macos](build/macos). See [INSTALL](INSTALL) for Clay build instructions.
