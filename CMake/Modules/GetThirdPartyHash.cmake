#
# Copyright (c) 2025-2025 the rbfx project.
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

# Generate a hash for ThirdParty directory based on git-tracked files and their current content
# This hash changes when:
# - Files are added/removed from git in the ThirdParty directory
# - Any tracked file content changes (even if not staged/committed)
# FNV-1a hash function
function(fnv1a_hash input output)
  set(_hash "2166136261")  # FNV offset basis (32-bit)
  string(LENGTH "${input}" _len)
  math(EXPR _len "${_len} - 1")

  foreach(_i RANGE 0 ${_len})
    string(SUBSTRING "${input}" ${_i} 1 _char)
    string(HEX "${_char}" _byte)
    math(EXPR _hash "(${_hash} ^ 0x${_byte}) * 16777619")
    math(EXPR _hash "${_hash} & 0xFFFFFFFF")  # Keep 32-bit
  endforeach()

  set(${output} ${_hash} PARENT_SCOPE)
endfunction()

function(get_thirdparty_hash DIRECTORY_PATH OUTPUT_VAR)
    # Parse optional HASH_FORMAT argument
    set(options "")
    set(oneValueArgs HASH_FORMAT)
    set(multiValueArgs "")
    cmake_parse_arguments(PARSE_ARGV 2 ARG "${options}" "${oneValueArgs}" "${multiValueArgs}")

    # Default format is "version"
    if(NOT ARG_HASH_FORMAT)
        set(ARG_HASH_FORMAT "version")
    endif()

    set(HASH_VALUE "0.0.0")

    # Check if we're in a git repository
    find_package(Git QUIET)
    if(NOT GIT_FOUND)
        if(ARG_HASH_FORMAT STREQUAL "short")
            set(${OUTPUT_VAR} "00000000" PARENT_SCOPE)
        else()
            message(WARNING "Git not found, ThirdParty hash will be '0.0.0'")
            set(${OUTPUT_VAR} ${HASH_VALUE} PARENT_SCOPE)
        endif()
        return()
    endif()

    # Check if directory exists
    if(NOT EXISTS "${DIRECTORY_PATH}")
        if(ARG_HASH_FORMAT STREQUAL "short")
            set(${OUTPUT_VAR} "00000000" PARENT_SCOPE)
        else()
            message(WARNING "ThirdParty directory '${DIRECTORY_PATH}' not found, hash will be '0.0.0'")
            set(${OUTPUT_VAR} ${HASH_VALUE} PARENT_SCOPE)
        endif()
        return()
    endif()

    # Get relative path from git root to the directory
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --show-toplevel
        WORKING_DIRECTORY "${DIRECTORY_PATH}"
        OUTPUT_VARIABLE GIT_ROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE GIT_RESULT
    )

    if(NOT GIT_RESULT EQUAL 0)
        if(ARG_HASH_FORMAT STREQUAL "short")
            set(${OUTPUT_VAR} "00000000" PARENT_SCOPE)
        else()
            message(WARNING "Not in a git repository, ThirdParty hash will be '0.0.0'")
            set(${OUTPUT_VAR} ${HASH_VALUE} PARENT_SCOPE)
        endif()
        return()
    endif()

    # Calculate relative path from git root to our directory
    file(RELATIVE_PATH REL_DIR "${GIT_ROOT}" "${DIRECTORY_PATH}")

    # Get list of all git-tracked files in the directory with their object info
    # This is equivalent to: git ls-files -s ${REL_DIR}
    execute_process(
        COMMAND ${GIT_EXECUTABLE} ls-files -s ${REL_DIR}
        WORKING_DIRECTORY "${GIT_ROOT}"
        OUTPUT_VARIABLE GIT_INDEX_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE GIT_RESULT
    )

    if(NOT GIT_RESULT EQUAL 0)
        if(ARG_HASH_FORMAT STREQUAL "short")
            set(${OUTPUT_VAR} "00000000" PARENT_SCOPE)
        else()
            message(WARNING "Failed to get git file list, ThirdParty hash will be '0.0.0'")
            set(${OUTPUT_VAR} ${HASH_VALUE} PARENT_SCOPE)
        endif()
        return()
    endif()

    # Parse the git ls-files output and collect current file hashes
    set(FILE_HASH_LIST "")
    string(REPLACE "\n" ";" GIT_LINES "${GIT_INDEX_OUTPUT}")

    foreach(LINE ${GIT_LINES})
        if(NOT LINE STREQUAL "")
            # Parse line format: <mode> <object> <stage> <file>
            string(REGEX MATCH "^([0-9]+) ([a-f0-9]+) ([0-9]+)\t(.+)$" MATCHED "${LINE}")
            if(MATCHED)
                set(FILE_MODE "${CMAKE_MATCH_1}")
                set(INDEX_HASH "${CMAKE_MATCH_2}")
                set(STAGE "${CMAKE_MATCH_3}")
                set(FILE_PATH "${CMAKE_MATCH_4}")

                # Get absolute path to file
                set(FULL_FILE_PATH "${GIT_ROOT}/${FILE_PATH}")

                # Calculate current file content hash (this includes local modifications)
                if(EXISTS "${FULL_FILE_PATH}")
                    file(SHA1 "${FULL_FILE_PATH}" CURRENT_HASH)
                else()
                    # File was deleted locally
                    set(CURRENT_HASH "deleted")
                endif()

                # Create entry similar to git ls-files -s but with current content hash
                list(APPEND FILE_HASH_LIST "${FILE_MODE} ${CURRENT_HASH} ${STAGE}\t${FILE_PATH}")
            endif()
        endif()
    endforeach()

    # Also include names of files with working-tree modifications in the directory
    # so that any file tree change (modify/move/delete) affects the version.
    # Equivalent to: git diff --name-only -- <REL_DIR>
    set(DIFF_FILE_LIST "")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} diff --name-only -- ${REL_DIR}
        WORKING_DIRECTORY "${GIT_ROOT}"
        OUTPUT_VARIABLE GIT_DIFF_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE GIT_DIFF_RESULT
    )
    if(GIT_DIFF_RESULT EQUAL 0 AND GIT_DIFF_OUTPUT)
        string(REPLACE "\n" ";" GIT_DIFF_LINES "${GIT_DIFF_OUTPUT}")
        foreach(DIFF_PATH ${GIT_DIFF_LINES})
            if(NOT DIFF_PATH STREQUAL "")
                # Prefix entries to avoid collision with ls-files lines
                list(APPEND FILE_HASH_LIST "diff\t${DIFF_PATH}")
            endif()
        endforeach()
    endif()

    # Sort the list for deterministic results
    list(SORT FILE_HASH_LIST)

    # Join all entries and hash the result
    string(JOIN "\n" HASH_INPUT ${FILE_HASH_LIST})
    string(SHA1 FINAL_HASH "${HASH_INPUT}")

    # Return based on requested format
    if(ARG_HASH_FORMAT STREQUAL "short")
        # Return first 8 characters of the hash
        string(SUBSTRING "${FINAL_HASH}" 0 8 SHORT_HASH)
        set(${OUTPUT_VAR} "${SHORT_HASH}" PARENT_SCOPE)
    elseif(ARG_HASH_FORMAT STREQUAL "full")
        # Return full SHA1 hash
        set(${OUTPUT_VAR} "${FINAL_HASH}" PARENT_SCOPE)
    else()
        # Default: Convert hash to version components
        fnv1a_hash("${FINAL_HASH}" _hash_value)
        math(EXPR _major "(${_hash_value} >> 24) & 0xFF")
        math(EXPR _minor "(${_hash_value} >> 12) & 0xFFF")
        math(EXPR _patch "${_hash_value} & 0xFFF")
        set(${OUTPUT_VAR} "${_major}.${_minor}.${_patch}" PARENT_SCOPE)
    endif()
endfunction()

# If this script is run directly with cmake -P
if(CMAKE_SCRIPT_MODE_FILE)
    # Default to ThirdParty subdirectory relative to this script's location
    if(NOT DEFINED DIRECTORY_PATH)
        get_filename_component(_script_dir "${CMAKE_SCRIPT_MODE_FILE}" DIRECTORY)
        get_filename_component(_root_dir "${_script_dir}" DIRECTORY)
        get_filename_component(_root_dir "${_root_dir}" DIRECTORY)
        set(DIRECTORY_PATH "${_root_dir}/Source/ThirdParty")
    endif()

    # Default format is "short" when run as script
    if(NOT DEFINED HASH_FORMAT)
        set(HASH_FORMAT "short")
    endif()

    # Generate the hash with specified format
    get_thirdparty_hash("${DIRECTORY_PATH}" THIRDPARTY_HASH HASH_FORMAT ${HASH_FORMAT})
    message("${THIRDPARTY_HASH}")
endif()
