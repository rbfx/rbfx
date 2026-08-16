@echo off
setlocal enabledelayedexpansion

set ROOT_DIR=%~dp0..
for %%I in ("%ROOT_DIR%") do set ROOT_DIR=%%~fI
if "%BUILD_DIR%"=="" set BUILD_DIR=%ROOT_DIR%\build-package
if "%~1"=="" (set PROFILE=%ROOT_DIR%\packaging\profiles\WindowsShipping.json) else (set PROFILE=%~1)
if "%~2"=="" (set ASSET_ROOT=%ROOT_DIR%\bin\CoreData) else (set ASSET_ROOT=%~2)
if "%~3"=="" (set MANIFEST=%BUILD_DIR%\package-manifest.json) else (set MANIFEST=%~3)

cmake -S "%ROOT_DIR%" -B "%BUILD_DIR%" -DURHO3D_TOOLS=ON -DURHO3D_TESTING=OFF -DURHO3D_EDITOR=OFF -DURHO3D_PLAYER=OFF -DURHO3D_CSHARP=OFF -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b %errorlevel%
cmake --build "%BUILD_DIR%" --target PackageBuilder --config Release --parallel
if errorlevel 1 exit /b %errorlevel%

set PACKAGE_BUILDER=%BUILD_DIR%\bin\Release\PackageBuilder.exe
if not exist "%PACKAGE_BUILDER%" set PACKAGE_BUILDER=%BUILD_DIR%\bin\PackageBuilder.exe
if not exist "%PACKAGE_BUILDER%" (
  echo PackageBuilder executable was not produced by CMake.
  exit /b 1
)

"%PACKAGE_BUILDER%" "%PROFILE%" "%ASSET_ROOT%" "%MANIFEST%"
if errorlevel 1 exit /b %errorlevel%
echo Package manifest: %MANIFEST%
endlocal
