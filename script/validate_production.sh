#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-validation}"
JOBS="${JOBS:-2}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DURHO3D_TESTING=ON \
  -DURHO3D_EDITOR=OFF \
  -DURHO3D_PLAYER=OFF \
  -DURHO3D_CSHARP=OFF
cmake --build "${BUILD_DIR}" --target Tests --parallel "${JOBS}"
ctest --test-dir "${BUILD_DIR}" --output-on-failure

PACKAGE_BUILDER="${BUILD_DIR}/bin/Release/PackageBuilder"
if [[ ! -x "${PACKAGE_BUILDER}" ]]; then
  PACKAGE_BUILDER="${BUILD_DIR}/bin/Debug/PackageBuilder"
fi
if [[ -x "${PACKAGE_BUILDER}" && -f "${ROOT_DIR}/packaging/profiles/LinuxDevelopment.json" ]]; then
  MANIFEST="${BUILD_DIR}/package-manifest.json"
  ASSET_ROOT="${ASSET_ROOT:-${ROOT_DIR}/bin/CoreData}"
  "${PACKAGE_BUILDER}" "${ROOT_DIR}/packaging/profiles/LinuxDevelopment.json" "${ASSET_ROOT}" "${MANIFEST}"
  test -s "${MANIFEST}"
  printf 'Validated package manifest: %s\n' "${MANIFEST}"
else
  printf 'PackageBuilder not present in this test build; packaging smoke test skipped.\n'
fi

printf 'Production validation completed successfully.\n'
