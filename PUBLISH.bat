@echo off
title POCKET METAL - build and publish on GitHub
rem One click: builds POCKET METAL (BUILD.bat), then puts this version on your GitHub (isod89/pocket-metal):
rem   the source (branch main = exactly this folder's files, tag v2), the release (the two .fwsc packages),
rem   and the web installer on GitHub Pages: https://isod89.github.io/pocket-metal/
rem Usage: PUBLISH.bat                  everything
rem        PUBLISH.bat -SourceOnly      the source only, without building
cd /d "%~dp0"
if /i "%~1"=="-SourceOnly" goto publish
echo == Step 1: build
call "%~dp0BUILD.bat" nopause
if errorlevel 1 (
  echo.
  echo The build failed: the release and the installer cannot be published.
  pause
  exit /b 1
)
:publish
echo.
echo == Step 2: publish on GitHub
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\publish.ps1" %*
pause
