@echo off
setlocal
set "DLL=build\Release\AtlasRuntime.dll"
if not exist "%DLL%" (echo Build AtlasRuntime first.& pause& exit /b 1)
if "%FNV_DIR%"=="" set /p FNV_DIR=Enter Fallout New Vegas game folder: 
set "DEST=%FNV_DIR%\Data\NVSE\Plugins"
if not exist "%DEST%" mkdir "%DEST%"
copy /y "%DLL%" "%DEST%\AtlasRuntime.dll" || exit /b 1
echo Installed to %DEST%
pause
