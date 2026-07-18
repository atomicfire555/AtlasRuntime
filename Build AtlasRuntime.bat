@echo off
setlocal

rem xNVSE repository root—the folder containing the "nvse" directory
set "XNVSE_SDK=C:\xNVSE\NVSE"

rem Change this to your Fallout New Vegas installation folder
set "FNV_DIR=F:\SteamLibrary\steamapps\common\Fallout New Vegas"

set "PLUGIN_DIR=%FNV_DIR%\Data\NVSE\Plugins"
set "BUILT_DLL=%CD%\build\Release\AtlasRuntime.dll"
set "INSTALLED_DLL=%PLUGIN_DIR%\AtlasRuntime.dll"

echo xNVSE source path:
echo "%XNVSE_SDK%"
echo.

if not exist "%XNVSE_SDK%\nvse\PluginAPI.h" (
echo ERROR: PluginAPI.h was not found.
echo Expected:
echo "%XNVSE_SDK%\nvse\PluginAPI.h"
pause
exit /b 1
)

if not exist "%FNV_DIR%\FalloutNV.exe" (
echo ERROR: Fallout New Vegas was not found.
echo Expected:
echo "%FNV_DIR%\FalloutNV.exe"
echo.
echo Edit FNV_DIR near the top of this script.
pause
exit /b 1
)

where cmake >nul 2>nul
if errorlevel 1 (
echo ERROR: CMake was not found in PATH.
pause
exit /b 1
)

rem Remove the old CMake cache and old build output
if exist build rmdir /s /q build

cmake -S . -B build -A Win32 -DXNVSE_SDK:PATH="%XNVSE_SDK%"
if errorlevel 1 goto :fail

cmake --build build --config Release
if errorlevel 1 goto :fail

if not exist "%BUILT_DLL%" (
echo ERROR: Build completed, but AtlasRuntime.dll was not found.
echo Expected:
echo "%BUILT_DLL%"
pause
exit /b 1
)

if not exist "%PLUGIN_DIR%" mkdir "%PLUGIN_DIR%"

copy /y "%BUILT_DLL%" "%INSTALLED_DLL%"
if errorlevel 1 goto :fail

echo.
echo Build and installation successful.
echo Installed:
echo "%INSTALLED_DLL%"
pause
exit /b 0

:fail
echo.
echo AtlasRuntime build or installation failed.
pause
exit /b 1
