@echo off
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION

rem ───────────────────────────────────────────────────────────────────────────
rem Usage:
rem   run_config_test.bat [--host 127.0.0.1] [--port 5432] [--level info] [--cli]
rem Examples:
rem   run_config_test.bat --host 127.0.0.1 --port 5432 --level debug
rem   run_config_test.bat --level warn --cli
rem ───────────────────────────────────────────────────────────────────────────

rem defaults
set HOST=127.0.0.1
set PORT=5432
set LEVEL=info
set MODE=env

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="--host"  ( set "HOST=%~2" & shift & shift & goto parse_args )
if /I "%~1"=="--port"  ( set "PORT=%~2" & shift & shift & goto parse_args )
if /I "%~1"=="--level" ( set "LEVEL=%~2" & shift & shift & goto parse_args )
if /I "%~1"=="--cli"   ( set "MODE=cli"  & shift & goto parse_args )
echo Unknown option: %~1
exit /b 2
:args_done

rem locate EXE (по умолчанию: ..\x64\Release\MSG5_Config_Tests.exe от папки батника)
set "SCRIPT_DIR=%~dp0"
set "BUILD_CFG=Release"
set "TARGET_DIR=%SCRIPT_DIR%x64\%BUILD_CFG%"
set "EXE=%TARGET_DIR%\MSG5_Config_Tests.exe"

if not exist "%EXE%" (
  echo [ERROR] Executable not found: "%EXE%"
  echo         Adjust TARGET_DIR/BUILD_CFG or build the project.
  exit /b 3
)

echo ------------------------------------------------------------
echo MSG5_Config_Tests launcher
echo   Mode   : %MODE%
echo   Host   : %HOST%
echo   Port   : %PORT%
echo   Level  : %LEVEL%
echo   EXE    : %EXE%
echo ------------------------------------------------------------

if /I "%MODE%"=="cli" (
  rem CLI has higher priority than ENV; run without touching ENV
  "%EXE%" --db.host=%HOST% --db.port=%PORT% --log.level=%LEVEL%
  set "RC=%ERRORLEVEL%"
  echo ------------------------------------------------------------
  echo Exit code: %RC%
  exit /b %RC%
) else (
  rem ENV mode (variables live only during this script due to setlocal)
  set "MSG5_DB_HOST=%HOST%"
  set "MSG5_DB_PORT=%PORT%"
  set "MSG5_LOG_LEVEL=%LEVEL%"

  echo Using ENV:
  echo   MSG5_DB_HOST=%MSG5_DB_HOST%
  echo   MSG5_DB_PORT=%MSG5_DB_PORT%
  echo   MSG5_LOG_LEVEL=%MSG5_LOG_LEVEL%
  echo ------------------------------------------------------------

  "%EXE%"
  set "RC=%ERRORLEVEL%"
  echo ------------------------------------------------------------
  echo Exit code: %RC%
  exit /b %RC%
)
