## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

ADD_LIBRARY(math STATIC constants.cpp)
SET_PROPERTY(TARGET math PROPERTY FOLDER common)
SET_PROPERTY(TARGET math APPEND PROPERTY COMPILE_FLAGS " ${FLAGS_LOWEST}")

IF (EMBREE_STATIC_LIB)
  # Urho3D: Fix exports
  install_third_party_libs(math)
  #INSTALL(EXPORT math-targets DESTINATION "${EMBREE_CMAKEEXPORT_DIR}" COMPONENT devel)
ENDIF()

