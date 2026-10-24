# Point Clay's Windows Python interpreter at MSYS Python (has PyGObject + Clay typelibs).
param(
    [string]$ClayPrefix = (Resolve-Path (Join-Path $PSScriptRoot "..\..\_install")).Path,
    [string]$PythonExe = "C:\msys64\clang64\bin\python.exe"
)

$interpDir = Join-Path $ClayPrefix "lib\clay\3.0\interpreters"
$pythonw = $PythonExe -replace "python\.exe$", "pythonw.exe"
if (-not (Test-Path $pythonw)) { $pythonw = $PythonExe }

$content = @"
python=$pythonw
python3=$pythonw
/usr/bin/python=$pythonw
/usr/bin/python3=$pythonw
:Python:E::py::python3:
"@

foreach ($name in @("pyclay_win.interp", "pygimp_win.interp")) {
    $path = Join-Path $interpDir $name
    if (Test-Path $interpDir) {
        Set-Content -Path $path -Value $content -Encoding ASCII
        Write-Host "Updated $path -> $pythonw"
    }
}

$girepo = Join-Path $ClayPrefix "lib\girepository-1.0"
$msysGirepo = Join-Path (Split-Path $PythonExe -Parent) "..\lib\girepository-1.0" | ForEach-Object {
    try { (Resolve-Path $_).Path } catch { $null }
}
$giPath = if ($msysGirepo) { "$girepo;$msysGirepo" } else { $girepo }
$binPath = Join-Path $ClayPrefix "bin"
$msysBin = Split-Path $PythonExe -Parent

$envDir = Join-Path $ClayPrefix "lib\clay\3.0\environ"
$pyclayEnv = @"
PYTHONDONTWRITEBYTECODE=1
__COMPAT_LAYER=HIGHDPIAWARE
GI_TYPELIB_PATH=$giPath
: PATH=$binPath;$msysBin
"@
if (Test-Path $envDir) {
    Set-Content -Path (Join-Path $envDir "pyclay.env") -Value $pyclayEnv -Encoding ASCII
    Set-Content -Path (Join-Path $envDir "pygimp.env") -Value $pyclayEnv -Encoding ASCII
    Write-Host "Updated pyclay.env (GI_TYPELIB_PATH + PATH for plug-ins)"
}

$mcpPy = @(
    (Join-Path $ClayPrefix "lib\clay\3.0\extensions\xyz.kainzmayer.clay.extension.mcp-bridge\clay-mcp-bridge.py"),
    (Join-Path $ClayPrefix "lib\clay\3.0\plug-ins\clay-mcp-bridge\clay-mcp-bridge.py")
)
foreach ($f in $mcpPy) {
    if (Test-Path $f) { (Get-Item $f).LastWriteTime = Get-Date }
}
