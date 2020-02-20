## ======================================================================== ##
## Copyright 2009-2018 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

SET(CMAKE_THREAD_PREFER_PTHREAD TRUE)
FIND_PACKAGE(Threads REQUIRED)
# Urho3D: Rename target and hide it in a subfolder.
ADD_LIBRARY(embree-sys STATIC
  sysinfo.cpp
  alloc.cpp
  filename.cpp
  library.cpp
  thread.cpp
  string.cpp
  regression.cpp
  mutex.cpp
  condition.cpp
  barrier.cpp
)

TARGET_LINK_LIBRARIES(embree-sys ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
SET_PROPERTY(TARGET embree-sys PROPERTY FOLDER ThirdParty/embree3/common)
SET_PROPERTY(TARGET embree-sys APPEND PROPERTY COMPILE_FLAGS " ${FLAGS_LOWEST}")

# Urho3D: Install targets
if (NOT URHO3D_MERGE_STATIC_LIBS)
    install(TARGETS embree-sys EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR})
endif ()
