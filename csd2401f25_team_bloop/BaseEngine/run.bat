@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem ==========================================================================
rem  PulseProtocol / PulseEngine / PulseEditor - Single configure/build/run
rem
rem  Put this file next to the TOP-LEVEL CMakeLists.txt (your BaseEngine root).
rem
rem  Usage:
rem    run.bat [editor|protocol|game|all|build|configure|clean|sln] [Config] [flags]
rem
rem  Config:
rem    Debug (default) | Release | RelWithDebInfo | MinSizeRel
rem
rem  Flags:
rem    --reconfigure   Force CMake configure step
rem    --clean         Delete build cache (CMakeCache.txt + CMakeFiles)
rem    --norun         Build only (don’t run the exe)
rem
rem  Examples:
rem    run.bat editor Debug
rem    run.bat protocol Release
rem    run.bat all Debug --reconfigure
rem    run.bat build Debug --clean
rem    run.bat sln
rem ==========================================================================

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "BUILD_DIR=%ROOT%\build"
set "GEN=Visual Studio 17 2022"

set "MODE=%~1"
if "%MODE%"=="" set "MODE=editor"
set "CONFIG=%~2"
if "%CONFIG%"=="" set "CONFIG=Debug"

set "RECONFIG=0"
set "CLEAN_CACHE=0"
set "NO_RUN=0"
for %%I in (%*) do (
  if /I "%%~I"=="--reconfigure" set "RECONFIG=1"
  if /I "%%~I"=="--clean" set "CLEAN_CACHE=1"
  if /I "%%~I"=="--norun" set "NO_RUN=1"
)

echo ROOT      = [%ROOT%]
echo BUILD_DIR = [%BUILD_DIR%]
echo.

if not exist "%ROOT%\CMakeLists.txt" (
  echo ERROR: CMakeLists.txt not found in [%ROOT%]
  echo Put run.bat in the SAME folder as the top-level CMakeLists.txt.
  pause
  exit /b 1
)

if /I "%MODE%"=="clean" (
  echo Cleaning full build folder...
  if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
  echo Done.
  exit /b 0
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

if "%CLEAN_CACHE%"=="1" (
  echo Cleaning CMake cache...
  if exist "%BUILD_DIR%\CMakeCache.txt" del /f /q "%BUILD_DIR%\CMakeCache.txt"
  if exist "%BUILD_DIR%\CMakeFiles" rmdir /s /q "%BUILD_DIR%\CMakeFiles"
  echo.
)

set "NEED_CONFIGURE=0"
if "%RECONFIG%"=="1" set "NEED_CONFIGURE=1"
if not exist "%BUILD_DIR%\CMakeCache.txt" set "NEED_CONFIGURE=1"

if "%NEED_CONFIGURE%"=="1" (
  echo Configuring...
  cmake -G "%GEN%" -A x64 -S "%ROOT%" -B "%BUILD_DIR%"
  if errorlevel 1 goto :fail
  echo.
) else (
  echo Configuring... (skipped, cache exists)
  echo.
)

if /I "%MODE%"=="configure" (
  echo Configure-only requested. Done.
  exit /b 0
)

if /I "%MODE%"=="sln" (
  echo Opening solution...
  for %%F in ("%BUILD_DIR%\*.sln") do ( start "" "%%~fF" & exit /b 0 )
  echo ERROR: No .sln found in [%BUILD_DIR%]
  pause
  exit /b 1
)

rem ---- Choose build targets (best-effort; falls back to ALL_BUILD) ----
set "TARGETS=ALL_BUILD"
set "RUN_EXE="

if /I "%MODE%"=="editor" (
  set "TARGETS=PulseEditor"
  set "RUN_EXE=PulseEditor"
) else if /I "%MODE%"=="protocol" (
  set "TARGETS=PulseProtocol"
  set "RUN_EXE=PulseProtocol"
) else if /I "%MODE%"=="game" (
  set "TARGETS=PulseProtocol"
  set "RUN_EXE=PulseProtocol"
) else if /I "%MODE%"=="all" (
  set "TARGETS=ALL_BUILD"
) else if /I "%MODE%"=="build" (
  set "TARGETS=ALL_BUILD"
) else (
  echo Unknown mode: %MODE%
  echo Valid modes: editor, protocol, game, all, build, configure, clean, sln
  pause
  exit /b 1
)

echo Building %CONFIG%... (target: %TARGETS%)
cmake --build "%BUILD_DIR%" --config "%CONFIG%" --target %TARGETS%
if errorlevel 1 goto :fail

if "%NO_RUN%"=="1" (
  echo.
  echo Build-only requested (--norun). Done.
  exit /b 0
)

if "%RUN_EXE%"=="" (
  echo.
  echo Built successfully. (Nothing to run for mode '%MODE%')
  exit /b 0
)

echo.
echo Locating %RUN_EXE%.exe ...
call :FindExe "%RUN_EXE%" "%CONFIG%"
if not defined FOUND_EXE (
  echo ERROR: Could not find %RUN_EXE%.exe under [%BUILD_DIR%]
  echo Tip: open the solution (run.bat sln) and check the target name/output.
  pause
  exit /b 1
)

echo Running:
echo   %FOUND_EXE%
echo.

pushd "%~dp0"
pushd "%FOUND_EXE_DIR%"
"%FOUND_EXE%"
set "APP_EXIT=%ERRORLEVEL%"
popd
popd
exit /b %APP_EXIT%

:FindExe
rem %1 = exe base name, %2 = config
set "FOUND_EXE="
set "FOUND_EXE_DIR="
set "_NAME=%~1"
set "_CFG=%~2"

rem Common CMake (VS multi-config) layouts:
set "CAND1=%BUILD_DIR%\%_CFG%\%_NAME%.exe"
set "CAND2=%BUILD_DIR%\%_NAME%\%_CFG%\%_NAME%.exe"
set "CAND3=%BUILD_DIR%\bin\%_CFG%\%_NAME%.exe"

if exist "%CAND1%" ( set "FOUND_EXE=%CAND1%" & set "FOUND_EXE_DIR=%BUILD_DIR%\%_CFG%" & goto :eof )
if exist "%CAND2%" ( set "FOUND_EXE=%CAND2%" & set "FOUND_EXE_DIR=%BUILD_DIR%\%_NAME%\%_CFG%" & goto :eof )
if exist "%CAND3%" ( set "FOUND_EXE=%CAND3%" & set "FOUND_EXE_DIR=%BUILD_DIR%\bin\%_CFG%" & goto :eof )

rem Fallback: search for *\<CFG>\<NAME>.exe
for /f "delims=" %%F in ('dir /b /s "%BUILD_DIR%\*\%_CFG%\%_NAME%.exe" 2^>nul') do (
  set "FOUND_EXE=%%F"
  for %%D in ("%%~dpF") do set "FOUND_EXE_DIR=%%~fD"
  goto :eof
)

goto :eof

:fail
echo.
echo BUILD FAILED
pause
exit /b 1
