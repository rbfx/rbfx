#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-package}"
PROFILE="${1:-${ROOT_DIR}/packaging/profiles/LinuxDevelopment.json}"
ASSET_ROOT="${2:-${ROOT_DIR}/bin/CoreData}"
MANIFEST="${3:-${BUILD_DIR}/package-manifest.json}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DURHO3D_TOOLS=ON \
  -DURHO3D_TESTING=OFF \
  -DURHO3D_EDITOR=OFF \
  -DURHO3D_PLAYER=OFF \
  -DURHO3D_CSHARP=OFF
cmake --build "${BUILD_DIR}" --target PackageBuilder --parallel

PACKAGE_BUILDER="${BUILD_DIR}/bin/Release/PackageBuilder"
if [[ ! -x "${PACKAGE_BUILDER}" ]]; then
  PACKAGE_BUILDER="${BUILD_DIR}/bin/PackageBuilder"
fi
if [[ ! -x "${PACKAGE_BUILDER}" ]]; then
  echo "PackageBuilder executable was not produced by CMake." >&2
  exit 1
fi

"${PACKAGE_BUILDER}" "${PROFILE}" "${ASSET_ROOT}" "${MANIFEST}"
printf 'Package manifest: %s\n' "${MANIFEST}"
