@echo off
title POCKET METAL - build
REM Builds POCKET METAL: build\apps\metal\ (metal.bin, metal-PocketX_902.fwsc, metal-BlackBox_902.fwsc, manifest.json).
REM  - with Node.js and Docker Desktop running: in the pinned container (node tools\op.mjs build);
REM  - otherwise in WSL, with the JieLi toolchain installed there once (wsl sh firmware/setup.sh).
REM Usage: BUILD.bat [docker^|wsl] [nopause]
setlocal
cd /d "%~dp0"
set WAY=
set NOPAUSE=
for %%a in (%*) do (
  if /i "%%~a"=="nopause" set NOPAUSE=1
  if /i "%%~a"=="docker" set WAY=docker
  if /i "%%~a"=="wsl" set WAY=wsl
)
if defined WAY goto %WAY%
where node >nul 2>&1 || goto auto_wsl
docker info >nul 2>&1 || goto auto_wsl

:docker
where node >nul 2>&1 || ( echo Node.js 18 or newer is needed: https://nodejs.org & goto fail )
echo == Building POCKET METAL in the container (Docker)
node tools\op.mjs build || goto fail
goto ok

:auto_wsl
echo Docker Desktop or Node.js is not available: building in WSL instead.
:wsl
where wsl >nul 2>&1 || ( echo Neither Docker Desktop + Node.js nor WSL is installed: see README.md, "Build it yourself". & goto fail )
if exist "build\apps\metal\metal.bin" del /q "build\apps\metal\metal.bin"
echo == Building POCKET METAL in WSL (JieLi toolchain from firmware/setup.sh)
wsl --cd "%~dp0firmware" -- env OP_APP=metal sh build.sh
if not exist "build\apps\metal\metal.bin" goto fail

:ok
echo.
echo SUCCESS: POCKET METAL v2
dir /b "build\apps\metal\*.fwsc"
if not defined NOPAUSE pause
exit /b 0

:fail
echo.
echo BUILD FAILED: read the messages above.
if not defined NOPAUSE pause
exit /b 1
