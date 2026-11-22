@echo off
title POCKET METAL - installer on this computer
REM The installer page on this computer (http://localhost:8765/), with the packages BUILD.bat made: no internet needed.
REM The public installer is https://isod89.github.io/pocket-metal/
setlocal
cd /d "%~dp0"
if not exist "build\apps\metal\manifest.json" (
  echo == No package yet: building first
  call "%~dp0BUILD.bat" nopause || goto fail
)
where py >nul 2>&1 && ( py -3 tools\serve.py & goto end )
where python >nul 2>&1 && ( python tools\serve.py & goto end )
echo Python 3 is needed to serve the installer offline: https://www.python.org/downloads/
:fail
echo.
echo FAILED.
:end
pause
