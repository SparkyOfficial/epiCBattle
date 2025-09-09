@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Usage: build_ninja.bat [Debug|Release] [run]
set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

echo === epiCBattle: Ninja CMake build (%BUILD_TYPE%) ===

set CMAKE_GEN_ARGS=-G "Ninja"
if exist "%CD%\ninja.exe" goto have_ninja
goto after_ninja

:have_ninja
set "NINJA_EXE=%CD%\ninja.exe"
echo Found local ninja: %NINJA_EXE%
rem Add current dir to PATH so CMake can find ninja by name
set "PATH=%CD%;%PATH%"
rem Use forward slashes for CMAKE_MAKE_PROGRAM to avoid escaping issues
set "NINJA_EXE_SLASH=%NINJA_EXE:\=/%"
set "CMAKE_GEN_ARGS=%CMAKE_GEN_ARGS% -DCMAKE_MAKE_PROGRAM=%NINJA_EXE_SLASH%"

:after_ninja

rem Prefer lightweight MSYS2 MinGW if present
set "MINGW_BIN=C:\msys64\mingw64\bin"
if exist "%MINGW_BIN%\g++.exe" goto use_mingw
goto after_mingw

:use_mingw
echo Using MSYS2 MinGW64 toolchain at %MINGW_BIN%
set "PATH=%MINGW_BIN%;%PATH%"
set "CMAKE_GEN_ARGS=%CMAKE_GEN_ARGS% -DCMAKE_C_COMPILER=%MINGW_BIN%\gcc.exe -DCMAKE_CXX_COMPILER=%MINGW_BIN%\g++.exe"

:after_mingw

set "BUILD_DIR=build-ninja"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
rem Clear cache if toolchain or generator changes
if exist "%BUILD_DIR%\CMakeCache.txt" del /f /q "%BUILD_DIR%\CMakeCache.txt" >nul 2>&1
if exist "%BUILD_DIR%\CMakeFiles" rmdir /s /q "%BUILD_DIR%\CMakeFiles" >nul 2>&1

cmake -S . -B "%BUILD_DIR%" %CMAKE_GEN_ARGS% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_POLICY_VERSION_MINIMUM=3.5
if errorlevel 1 (
  echo [ERROR] CMake configure failed.
  exit /b 1
)

cmake --build "%BUILD_DIR%"
if errorlevel 1 (
  echo [ERROR] Build failed.
  exit /b 1
)

set "EXE_PATH=%BUILD_DIR%\epiCBattle.exe"
if not exist "%EXE_PATH%" (
  echo [WARN] Executable not found at expected location: %EXE_PATH%
  exit /b 1
)

echo.
echo Build succeeded: %EXE_PATH%

if /I "%2"=="run" (
  echo Running...
  "%EXE_PATH%"
)

endlocal
exit /b 0


