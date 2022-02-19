#
# Copyright (c) 2008-2022 the Urho3D project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

# Find Direct Rendering Manager development library
#
#  DRM_FOUND
#  DRM_INCLUDE_DIRS
#  DRM_LIBRARIES
#

find_path (DRM_INCLUDE_DIRS NAMES drm.h PATH_SUFFIXES libdrm DOC "DirectRenderingManager include directory")
find_library (DRM_LIBRARIES NAMES drm DOC "DirectRenderingManager library")

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (DRM REQUIRED_VARS DRM_LIBRARIES DRM_INCLUDE_DIRS FAIL_MESSAGE "Could NOT find Direct Rendering Manager development library")

mark_as_advanced (DRM_INCLUDE_DIRS DRM_LIBRARIES)
