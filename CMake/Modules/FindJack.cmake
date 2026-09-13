# Copyright (c) 2008-2022 the Urho3D project.
# Copyright (c) 2022-2026 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

# Find Jack Audio Connection Kit development library
#
#  JACK_FOUND
#  JACK_INCLUDE_DIRS
#  JACK_LIBRARIES
#

find_path (JACK_INCLUDE_DIRS NAMES jack/jack.h DOC "JACK include directory")
find_library (JACK_LIBRARIES NAMES jack DOC "JACK library")

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (Jack REQUIRED_VARS JACK_LIBRARIES JACK_INCLUDE_DIRS FAIL_MESSAGE "Could NOT find Jack Audio Connection Kit development library")

mark_as_advanced (JACK_INCLUDE_DIRS JACK_LIBRARIES)
