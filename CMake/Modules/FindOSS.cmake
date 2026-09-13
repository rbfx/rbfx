# Copyright (c) 2008-2022 the Urho3D project.
# Copyright (c) 2022-2026 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

# Find Open Sound System development library
#
#  OSS_FOUND
#  OSS_INCLUDE_DIRS
#  OSS_LIBRARIES
#  OSS_USE_WORKAROUND_HEADER
#

find_path (OSS_INCLUDE_DIRS NAMES sys/soundcard.h soundcard.h PATH_SUFFIXES uClibc DOC "OSS include directory")
find_library (OSS_LIBRARIES NAMES OSSlib ossaudio DOC "OSS library")
find_file (OSS_USE_WORKAROUND_HEADER NAMES soundcard.h DOC "OSS use workaround header")

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (OSS REQUIRED_VARS OSS_LIBRARIES OSS_INCLUDE_DIRS FAIL_MESSAGE "Could NOT find OSS development library")

mark_as_advanced (OSS_INCLUDE_DIRS OSS_LIBRARIES OSS_USE_WORKAROUND_HEADER)
