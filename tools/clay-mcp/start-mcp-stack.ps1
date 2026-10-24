# Configure Clay, refresh MCP plug-in cache, launch Clay, verify bridge TCP port.
param(
    [int]$Port = 9877,
    [int]$WaitSeconds = 120
)

$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
& (Join-Path $PSScriptRoot "configure-windows-python.ps1") -ClayPrefix (Join-Path $root "_install")

$src = Join-Path $root "extensions\clay-mcp-bridge\clay-mcp-bridge.py"
$targets = @(
    (Join-Path $root "_install\lib\clay\3.0\extensions\xyz.kainzmayer.clay.extension.mcp-bridge\clay-mcp-bridge.py"),
    (Join-Path $root "_install\lib\clay\3.0\plug-ins\clay-mcp-bridge\clay-mcp-bridge.py")
)
New-Item -ItemType Directory -Force -Path (Split-Path $targets[1] -Parent) | Out-Null
foreach ($t in $targets) { Copy-Item -Force $src $t }

Get-Process clay* -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
$pluginrc = Join-Path $env:APPDATA "Clay\3.36\pluginrc"
if (Test-Path $pluginrc) { Remove-Item $pluginrc -Force }

$env:CLAY_MCP_AUTOSTART = "1"
& (Join-Path $PSScriptRoot "start-clay.ps1")

Write-Host @"

Clay is starting (first launch rescans plug-ins; can take 1-3 minutes).
Then open any image and choose:
  Tools -> AI Agent Bridge (MCP Server)...
Leave that dialog open so port $Port stays active for Cursor MCP.

Cursor: reload MCP server 'clay' (.cursor/mcp.json).
"@
