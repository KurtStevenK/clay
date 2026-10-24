@echo off
set "PATH=%~dp0_install\bin;C:\msys64\clang64\bin;%PATH%"
set "GI_TYPELIB_PATH=%~dp0_install\lib\girepository-1.0;C:\msys64\clang64\lib\girepository-1.0"
set "CLAY_MCP_AUTOSTART=1"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\clay-mcp\configure-windows-python.ps1" >nul 2>&1
start "" "%~dp0_install\bin\clay-3.36.exe" %*
