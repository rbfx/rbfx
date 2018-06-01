@echo off
rem This file makes your life easier.
rem Running this file will automatically pick appropriate cmake project generator
rem platform and LLVM paths. You may pass additional cmake arguments to this script
rem and they will be forwarded to cmake.

setlocal enabledelayedexpansion

set "CMAKE_ARGS=%*"

IF NOT EXIST build (
    mkdir build
)

if EXIST build\environment.cmd (
    call build\environment.cmd
    goto process
)

:prepare
if "!PROCESSOR_ARCHITECTURE!" == "AMD64" (
    set /P "PLATFORM=Platform (x86/_x64_): " || set "PLATFORM=x64"
) else (
    set "PLATFORM=x86"
)
echo set "PLATFORM=!PLATFORM!" > build\environment.cmd

set /P "VS=Visual Studio (2015/_2017_): " || set "VS=2017"
echo set "VS=!VS!" >> build\environment.cmd

if "!LLVM_VERSION!" == "" (
    set /P LLVM_VERSION="LLVM version: "
    echo set "LLVM_VERSION=!LLVM_VERSION!" >> build\environment.cmd
)

:process
if NOT "!LLVM_VERSION!" == "" (
    if "!PLATFORM!" == "x86" (
        set "LLVM_DIR=!ProgramFiles(x86)!\LLVM"
    ) else (
        set "LLVM_DIR=!ProgramFiles!\LLVM"
    )
    set "LLVM_DIR=!LLVM_DIR:\=/!"
    set CMAKE_ARGS=!CMAKE_ARGS! -DLLVM_VERSION_EXPLICIT=!LLVM_VERSION! -DLIBCLANG_LIBRARY="!LLVM_DIR!/lib/libclang.lib" -DLIBCLANG_INCLUDE_DIR="!LLVM_DIR!/include" -DLIBCLANG_SYSTEM_INCLUDE_DIR="!LLVM_DIR!/lib/clang/!LLVM_VERSION!/include" -DCLANG_BINARY="!LLVM_DIR!/bin/clang++.exe"
    echo Using LLVM !LLVM_VERSION! from !LLVM_DIR!
)

set "CMAKE_GENRATOR=Visual Studio"
if "!VS!" == "2015" (
    set "CMAKE_GENRATOR=!CMAKE_GENRATOR! 14"
) else if "!VS!" == "2017" (
    set "CMAKE_GENRATOR=!CMAKE_GENRATOR! 15"
) else (
    echo Unknown Visual Studio version
    goto quit
)
set "CMAKE_GENRATOR=!CMAKE_GENRATOR! !vs!"

if "!PLATFORM!" == "x64" (
    set "CMAKE_GENRATOR=!CMAKE_GENRATOR! Win64"
) else if NOT "!PLATFORM!" == "x86" (
    echo Unknown platform
    goto quit
)

echo Using !CMAKE_GENRATOR! on !PLATFORM!

:build
pushd build
echo cmake.exe -G "!CMAKE_GENRATOR!" !CMAKE_ARGS! ..
cmake.exe -G "!CMAKE_GENRATOR!" !CMAKE_ARGS! ..
popd

:quit
@pause
