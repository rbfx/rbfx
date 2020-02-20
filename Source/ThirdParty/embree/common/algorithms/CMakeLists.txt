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
# Urho3D: Rename target and hide it in a subfolder.
ADD_LIBRARY(embree-algorithms OBJECT
 parallel_for.cpp
 parallel_reduce.cpp
 parallel_prefix_sum.cpp
 parallel_for_for.cpp
 parallel_for_for_prefix_sum.cpp
 parallel_partition.cpp
 parallel_sort.cpp
 parallel_set.cpp
 parallel_map.cpp
 parallel_filter.cpp
)

SET_PROPERTY(TARGET embree-algorithms PROPERTY FOLDER ThirdParty/embree3/common)
SET_PROPERTY(TARGET embree-algorithms APPEND PROPERTY COMPILE_FLAGS " ${FLAGS_LOWEST}")
