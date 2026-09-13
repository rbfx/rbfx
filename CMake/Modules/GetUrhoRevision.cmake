# Copyright (c) 2008-2022 the Urho3D project.
# Copyright (c) 2022-2026 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

# Get Urho3D library revision number

# Use the same commit-ish used by CI server to describe the repository
if (DEFINED ENV{GITHUB_SHA})
    # GitHub Actions environment
    set (ARG $ENV{GITHUB_SHA})
else ()
    set (ARG --dirty)
endif ()

# Get git describe output, excluding nuget/* tags
execute_process (COMMAND git describe ${ARG} --exclude=nuget/* RESULT_VARIABLE GIT_EXIT_CODE OUTPUT_VARIABLE LIB_REVISION ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if (NOT GIT_EXIT_CODE EQUAL 0)
    # No GIT command line tool or not a GIT repository
    set (LIB_REVISION Unversioned)
endif ()
if (FILENAME)
    # Output complete revision number to a file
    file (WRITE ${FILENAME} "namespace Urho3D { static const char* revision = \"${LIB_REVISION}\"; };\n")
else ()
    # Output just major.minor.patch number to stdout
    string (REGEX MATCH "[^.]+\\.[^-]+" VERSION ${LIB_REVISION})            # Assume release tag always has major.minor format with possible pre-release identifier
    if (VERSION)
        string (REGEX MATCH "${VERSION}-([A-Z]+)" PRE_ID ${LIB_REVISION})   # Workaround as CMake's regex does not support look around
        if (PRE_ID)
            set (VERSION ${VERSION}-${CMAKE_MATCH_1})
        endif ()
        string (REGEX MATCH "${VERSION}-([^-]+)-g[0-9a-f]+" DEV_INFO ${LIB_REVISION})     # Check if we have commits after tag (development version)
        if (DEV_INFO)
            # Extract number of commits since tag
            string (REGEX MATCH "${VERSION}-([^-]+)" PATCH ${LIB_REVISION})
            set (VERSION ${VERSION}.${CMAKE_MATCH_1}-dev)
        else ()
            # Exact tag match, no development suffix
            set (VERSION ${VERSION}.0)
        endif ()
    else ()
        set (VERSION 0.0.0)
    endif ()
    execute_process (COMMAND ${CMAKE_COMMAND} -E echo ${VERSION})
endif ()
