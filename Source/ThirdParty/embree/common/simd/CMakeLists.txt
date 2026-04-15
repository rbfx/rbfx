## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

ADD_LIBRARY(simd STATIC sse.cpp)
SET_PROPERTY(TARGET simd PROPERTY FOLDER common)
SET_PROPERTY(TARGET simd APPEND PROPERTY COMPILE_FLAGS " ${FLAGS_LOWEST}")

IF (EMBREE_STATIC_LIB)
  # Urho3D: Fix exports
  install_third_party_libs(simd)
  #INSTALL(EXPORT simd-targets DESTINATION "${EMBREE_CMAKEEXPORT_DIR}" COMPONENT devel)
ENDIF()
