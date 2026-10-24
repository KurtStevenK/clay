# Launch Clay with PATH/GI settings so Python extensions (incl. MCP bridge) load.
# Usage: .\tools\clay-mcp\start-clay.ps1
# Optional: -StartMcpBridge opens Tools -> MCP bridge after launch (manual step in UI).

param(
    [string]$ClayPrefix = (Resolve-Path (Join-Path $PSScriptRoot "..\..\_install")).Path,
    [string]$MsysBin = "C:\msys64\clang64\bin"
)

$ErrorActionPreference = "Stop"
$clayBin = Join-Path $ClayPrefix "bin"
$clayExe = Join-Path $clayBin "clay.exe"
if (-not (Test-Path $clayExe)) {
    $clayExe = Join-Path $clayBin "clay-3.36.exe"
}
if (-not (Test-Path $clayExe)) {
    throw "Clay not found under $ClayPrefix\bin. Build and meson install first."
}

$girepo = @(
    (Join-Path $ClayPrefix "lib\girepository-1.0")
    (Join-Path $MsysBin "..\lib\girepository-1.0" | ForEach-Object { Resolve-Path $_ -ErrorAction SilentlyContinue })
) | Where-Object { $_ -and (Test-Path $_) }

$env:PATH = "$clayBin;$MsysBin;" + $env:PATH
if ($girepo.Count -gt 0) {
    $env:GI_TYPELIB_PATH = ($girepo -join ";")
}
# MCP autostart is Tools → "✓ Autostart MCP Server on Launch" (%APPDATA%\Clay\3.36\mcp-bridge.json).
# Optional override: $env:CLAY_MCP_AUTOSTART = "1" or "0"

Write-Host "Starting Clay from $clayExe"
Write-Host "GI_TYPELIB_PATH=$env:GI_TYPELIB_PATH"
Start-Process -FilePath $clayExe -WorkingDirectory $clayBin
