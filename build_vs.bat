@echo off
setlocal EnableDelayedExpansion

rem Usage: build_vs.bat [Debug|Release] [run]
set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

echo === epiCBattle: Visual Studio CMake build (%BUILD_TYPE%) ===

rem Try to locate VsDevCmd.bat using vswhere
set VSWHERE_EXE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
set VSDEV_CMD=
if exist "%VSWHERE_EXE%" (
  for /f "usebackq tokens=*" %%i in (`"%VSWHERE_EXE%" -latest -prerelease -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set VS_INSTALL=%%i
  )
  if defined VS_INSTALL (
    if exist "%VS_INSTALL%\Common7\Tools\VsDevCmd.bat" set VSDEV_CMD=%VS_INSTALL%\Common7\Tools\VsDevCmd.bat
  )
)

rem Fallback common paths if vswhere not present
if not defined VSDEV_CMD (
  if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" set VSDEV_CMD=%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat
)
if not defined VSDEV_CMD (
  if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat" set VSDEV_CMD=%ProgramFiles%\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat
)
if not defined VSDEV_CMD (
  if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" set VSDEV_CMD=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat
)

if not defined VSDEV_CMD (
  echo [ERROR] Could not find Visual Studio Developer Command Prompt (VsDevCmd.bat).
  echo         Please install Visual Studio 2022 with C++ workload or run this script from the "x64 Native Tools Command Prompt for VS".
  exit /b 1
)

echo Using VS developer environment: %VSDEV_CMD%
call "%VSDEV_CMD%" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 (
  echo [ERROR] Failed to initialize VS developer environment.
  exit /b 1
)

set BUILD_DIR=build-vs
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

rem Try VS 2022 generator first
cmake -S . -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
  echo VS 2022 generator failed, trying VS 2019...
  cmake -S . -B "%BUILD_DIR%" -G "Visual Studio 16 2019" -A x64
  if errorlevel 1 (
    echo [ERROR] CMake configure failed.
    exit /b 1
  )
)

cmake --build "%BUILD_DIR%" --config %BUILD_TYPE%
if errorlevel 1 (
  echo [ERROR] Build failed.
  exit /b 1
)

set EXE_PATH=%BUILD_DIR%\%BUILD_TYPE%\epiCBattle.exe
if not exist "%EXE_PATH%" (
  echo [WARN] Executable not found at expected location: %EXE_PATH%
  echo       Please check build output for errors.
  exit /b 1
)

echo.
echo Build succeeded: %EXE_PATH%
echo Models are copied next to the executable by CMake.

if /I "%2"=="run" (
  echo Running...
  "%EXE_PATH%"
)

endlocal
exit /b 0


