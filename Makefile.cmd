@echo off
rem This file makes your life easier.
rem Running this file will automatically pick appropriate cmake project generator
rem and they will be forwarded to cmake.

setlocal enabledelayedexpansion

set "CMAKE_ARGS=%*"

IF NOT EXIST cmake-build (
    mkdir cmake-build
)

if EXIST cmake-build\environment.cmd (
    call cmake-build\environment.cmd
    goto process
)

:prepare
if "!PROCESSOR_ARCHITECTURE!" == "AMD64" (
    set /P "PLATFORM=Platform (x86/[x64]): " || set "PLATFORM=x64"
) else (
    set "PLATFORM=x86"
)

echo set "PLATFORM=!PLATFORM!" > cmake-build\environment.cmd
set /P "VS=Visual Studio (2017/[2019]): " || set "VS=2019"
echo set "VS=!VS!" >> cmake-build\environment.cmd

:process
set "VS_PLATFORM=!PLATFORM!"
if "!VS_PLATFORM!" == "x86" (
	set "VS_PLATFORM=Win32"
)

set "CMAKE_GENRATOR=Visual Studio"
if "!VS!" == "2017" (
    set "CMAKE_GENRATOR=!CMAKE_GENRATOR! 15"
    call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" !PLATFORM!
) else if "!VS!" == "2019" (
    set "CMAKE_GENRATOR=!CMAKE_GENRATOR! 16"
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" !PLATFORM!
) else (
    echo Unknown Visual Studio version
    goto quit
)
set "CMAKE_GENRATOR=!CMAKE_GENRATOR! !vs!"

if "!VS!" == "2019" (
	set "CMAKE_GENRATOR=!CMAKE_GENRATOR!"
	set CMAKE_ARGS=-A !VS_PLATFORM! !CMAKE_ARGS!
) else if "!PLATFORM!" == "x64" (
	set "CMAKE_GENRATOR=!CMAKE_GENRATOR! Win64"
) else if NOT "!PLATFORM!" == "x86" (
    echo Unknown platform
    goto quit
)

echo Using !CMAKE_GENRATOR! on !PLATFORM!

:build
pushd cmake-build
echo cmake.exe -G "!CMAKE_GENRATOR!" !CMAKE_ARGS! ..
cmake.exe -G "!CMAKE_GENRATOR!" !CMAKE_ARGS! ..
echo "Platform: !VS_PLATFORM!"
msbuild rbfx.sln /p:Platform="!VS_PLATFORM!" /t:restore
popd

:quit
@pause
