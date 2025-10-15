## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

ADD_LIBRARY(lexers STATIC
 stringstream.cpp
 tokenstream.cpp
)
TARGET_LINK_LIBRARIES(lexers sys math)
SET_PROPERTY(TARGET lexers PROPERTY FOLDER common)
SET_PROPERTY(TARGET lexers APPEND PROPERTY COMPILE_FLAGS " ${FLAGS_LOWEST}")

IF (EMBREE_STATIC_LIB)
  # Urho3D: Fix exports
  install_third_party_libs(lexers)
  #INSTALL(EXPORT lexers-targets DESTINATION "${EMBREE_CMAKEEXPORT_DIR}" COMPONENT devel)
ENDIF()


