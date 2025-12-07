@echo off
setlocal enabledelayedexpansion

REM ---- 1) choose python launcher ----
where py >nul 2>nul
if %errorlevel%==0 (
  set "PY=py"
) else (
  where python >nul 2>nul
  if %errorlevel%==0 (
    set "PY=python"
  ) else (
    echo [ERROR] Python not found. Install Python first.
    exit /b 1
  )
)

echo Using: %PY%
%PY% -V

REM ---- 2) ensure pip ----
%PY% -m pip --version >nul 2>nul
if %errorlevel% neq 0 (
  echo pip not found, trying ensurepip...
  %PY% -m ensurepip --default-pip
  if %errorlevel% neq 0 (
    echo [ERROR] ensurepip failed. Install pip manually or reinstall Python with pip.
    exit /b 1
  )
)

REM ---- 3) upgrade pip ----
%PY% -m pip install -U pip

REM ---- 4) install matplotlib ----
%PY% -m pip install -U matplotlib

REM ---- 5) install your package ----
set "WHEEL="
for %%F in (*.whl) do (
  set "WHEEL=%%F"
  goto :havewheel
)

:havewheel
if not "%WHEEL%"=="" (
  echo Found wheel: %WHEEL%
  %PY% -m pip install --force-reinstall "%WHEEL%"
) else (
  echo No wheel found here, installing from current folder: pip install .
  %PY% -m pip install .
)

REM ---- 6) quick import test ----
%PY% -c "import rhythmic_delivery as rd; print('import ok')"
if %errorlevel% neq 0 (
  echo [ERROR] Import test failed.
  exit /b 1
)

echo Done.
endlocal
