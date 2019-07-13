#!/usr/bin/env sh

# Required by native build of PackageTool
apt-get update
apt-get -y install uuid-dev

SOURCE_DIR=$(pwd)
CI_DIR=$(dirname "$(readlink -f "$0")")
mkdir -p cmake-build
mkdir -p SDK

cd cmake-build
cmake -DURHO3D_SAMPLES=ON -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_INSTALL_PREFIX=$SOURCE_DIR/SDK -DCMAKE_TOOLCHAIN_FILE=$CI_DIR/../CMake/Toolchains/Emscripten.cmake $SOURCE_DIR
cmake --build . --target 11_Physics -- -j $(nproc --all)
