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

# Convert full SHA1 hash to a short 8-character hex string
# Uses XOR folding to incorporate all 40 characters of the hash
function(hash_to_short input output)
  # SHA1 produces 40 hex chars (160 bits). We want 8 hex chars (32 bits).
  # XOR-fold the hash: divide into 5 groups of 8 chars and XOR them together
  string(SUBSTRING "${input}" 0 8 _part1)
  string(SUBSTRING "${input}" 8 8 _part2)
  string(SUBSTRING "${input}" 16 8 _part3)
  string(SUBSTRING "${input}" 24 8 _part4)
  string(SUBSTRING "${input}" 32 8 _part5)

  # Convert to integers and XOR them
  math(EXPR _result "0x${_part1} ^ 0x${_part2} ^ 0x${_part3} ^ 0x${_part4} ^ 0x${_part5}" OUTPUT_FORMAT HEXADECIMAL)

  # Remove "0x" prefix and ensure 8 characters with leading zeros
  string(SUBSTRING "${_result}" 2 -1 _hex)
  string(LENGTH "${_hex}" _len)
  if(_len LESS 8)
    string(REPEAT "0" ${_len} _zeros)
    string(SUBSTRING "${_zeros}" 0 ${_len} _padding)
    math(EXPR _needed "8 - ${_len}")
    string(REPEAT "0" ${_needed} _padding)
    set(_hex "${_padding}${_hex}")
  endif()

  set(${output} "${_hex}" PARENT_SCOPE)
endfunction()

# Convert SHA1 hash to version number format
# Uses XOR folding to incorporate the entire hash into the version components
function(hash_to_version input output)
  # First get the XOR-folded short hash
  hash_to_short("${input}" _short_hash)

  # Split the 8-character hash into version components
  string(SUBSTRING "${_short_hash}" 0 2 _major_hex)
  string(SUBSTRING "${_short_hash}" 2 3 _minor_hex)
  string(SUBSTRING "${_short_hash}" 5 3 _patch_hex)

  # Convert to decimal
  math(EXPR _major "0x${_major_hex}")
  math(EXPR _minor "0x${_minor_hex}")
  math(EXPR _patch "0x${_patch_hex}")

  set(${output} "${_major}.${_minor}.${_patch}" PARENT_SCOPE)
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

    # Get list of all git-tracked files in the directory with their index hashes
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

    # Get list of modified files (files that differ from index)
    # This is MUCH faster than hashing every file individually
    execute_process(
        COMMAND ${GIT_EXECUTABLE} diff-files --name-only ${REL_DIR}
        WORKING_DIRECTORY "${GIT_ROOT}"
        OUTPUT_VARIABLE MODIFIED_FILES
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Convert modified files list to a set for O(1) lookup
    if(MODIFIED_FILES)
        string(REPLACE "\n" ";" MODIFIED_LINES "${MODIFIED_FILES}")
        foreach(MOD_FILE ${MODIFIED_LINES})
            set(MODIFIED_SET_${MOD_FILE} TRUE)
        endforeach()
    endif()

    # Collect files that need hashing (modified + deleted)
    set(FILES_TO_HASH "")
    set(FILE_HASH_LIST "")
    string(REPLACE "\n" ";" GIT_LINES "${GIT_INDEX_OUTPUT}")

    foreach(LINE ${GIT_LINES})
        # Parse line format: <mode> <object> <stage> <file>
        string(REGEX MATCH "^([0-9]+) ([a-f0-9]+) ([0-9]+)\t(.+)$" MATCHED "${LINE}")
        if(MATCHED)
            set(FILE_MODE "${CMAKE_MATCH_1}")
            set(INDEX_HASH "${CMAKE_MATCH_2}")
            set(STAGE "${CMAKE_MATCH_3}")
            set(FILE_PATH "${CMAKE_MATCH_4}")

            set(FULL_FILE_PATH "${GIT_ROOT}/${FILE_PATH}")

            # Check if file is modified or deleted
            if(NOT EXISTS "${FULL_FILE_PATH}")
                # File was deleted locally
                list(APPEND FILE_HASH_LIST "${FILE_MODE} deleted ${STAGE}\t${FILE_PATH}")
            elseif(DEFINED MODIFIED_SET_${FILE_PATH})
                # File is modified, collect it for batch hashing
                list(APPEND FILES_TO_HASH "${FULL_FILE_PATH}")
                list(APPEND FILES_TO_HASH_INFO "${FILE_MODE}|${STAGE}|${FILE_PATH}")
            else()
                # File unchanged, use index hash
                list(APPEND FILE_HASH_LIST "${FILE_MODE} ${INDEX_HASH} ${STAGE}\t${FILE_PATH}")
            endif()
        endif()
    endforeach()

    # Batch hash all modified files in one git command (huge performance win!)
    if(FILES_TO_HASH)
        # Write files to a temp file for stdin-paths
        string(JOIN "\n" FILES_CONTENT ${FILES_TO_HASH})
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/files_to_hash.txt" "${FILES_CONTENT}\n")

        execute_process(
            COMMAND ${GIT_EXECUTABLE} hash-object --stdin-paths
            WORKING_DIRECTORY "${GIT_ROOT}"
            INPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/files_to_hash.txt"
            OUTPUT_VARIABLE BATCH_HASHES
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE HASH_RESULT
        )

        if(HASH_RESULT EQUAL 0 AND BATCH_HASHES)
            string(REPLACE "\n" ";" HASH_LINES "${BATCH_HASHES}")
            list(LENGTH FILES_TO_HASH NUM_FILES)
            list(LENGTH HASH_LINES NUM_HASHES)

            if(NUM_FILES EQUAL NUM_HASHES)
                # Add hashed files to FILE_HASH_LIST with their actual hashes
                math(EXPR LAST_INDEX "${NUM_FILES} - 1")
                foreach(IDX RANGE 0 ${LAST_INDEX})
                    list(GET FILES_TO_HASH_INFO ${IDX} FILE_INFO)
                    list(GET HASH_LINES ${IDX} HASH_VALUE)
                    # Parse stored info: mode|stage|path
                    string(REPLACE "|" ";" INFO_PARTS "${FILE_INFO}")
                    list(GET INFO_PARTS 0 FILE_MODE)
                    list(GET INFO_PARTS 1 STAGE)
                    list(GET INFO_PARTS 2 FILE_PATH)
                    list(APPEND FILE_HASH_LIST "${FILE_MODE} ${HASH_VALUE} ${STAGE}\t${FILE_PATH}")
                endforeach()
            endif()
        endif()
    endif()

    # Also check for untracked files (these still need individual hashing, but typically fewer)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} ls-files --others --exclude-standard ${REL_DIR}
        WORKING_DIRECTORY "${GIT_ROOT}"
        OUTPUT_VARIABLE UNTRACKED_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE UNTRACKED_RESULT
    )
    if(UNTRACKED_RESULT EQUAL 0 AND UNTRACKED_OUTPUT)
        # Batch hash untracked files too
        set(UNTRACKED_FILES "")
        set(UNTRACKED_PATHS "")
        string(REPLACE "\n" ";" UNTRACKED_LINES "${UNTRACKED_OUTPUT}")
        foreach(UNTRACKED_PATH ${UNTRACKED_LINES})
            set(UNTRACKED_FULL_PATH "${GIT_ROOT}/${UNTRACKED_PATH}")
            if(EXISTS "${UNTRACKED_FULL_PATH}")
                list(APPEND UNTRACKED_FILES "${UNTRACKED_FULL_PATH}")
                list(APPEND UNTRACKED_PATHS "${UNTRACKED_PATH}")
            endif()
        endforeach()

        if(UNTRACKED_FILES)
            string(JOIN "\n" UNTRACKED_CONTENT ${UNTRACKED_FILES})
            file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/untracked_files.txt" "${UNTRACKED_CONTENT}\n")

            execute_process(
                COMMAND ${GIT_EXECUTABLE} hash-object --stdin-paths
                WORKING_DIRECTORY "${GIT_ROOT}"
                INPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/untracked_files.txt"
                OUTPUT_VARIABLE UNTRACKED_HASHES
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )

            if(UNTRACKED_HASHES)
                string(REPLACE "\n" ";" UNTRACKED_HASH_LINES "${UNTRACKED_HASHES}")
                list(LENGTH UNTRACKED_FILES NUM_UNTRACKED)
                list(LENGTH UNTRACKED_HASH_LINES NUM_UNTRACKED_HASHES)

                if(NUM_UNTRACKED EQUAL NUM_UNTRACKED_HASHES)
                    math(EXPR LAST_INDEX "${NUM_UNTRACKED} - 1")
                    foreach(IDX RANGE 0 ${LAST_INDEX})
                        list(GET UNTRACKED_PATHS ${IDX} FILE_PATH)
                        list(GET UNTRACKED_HASH_LINES ${IDX} HASH_VALUE)
                        list(APPEND FILE_HASH_LIST "untracked ${HASH_VALUE} 0\t${FILE_PATH}")
                    endforeach()
                endif()
            endif()
        endif()
    endif()

    # Sort the list for deterministic results
    list(SORT FILE_HASH_LIST)

    # Join all entries and hash the result
    string(JOIN "\n" HASH_INPUT ${FILE_HASH_LIST})
    string(SHA1 FINAL_HASH "${HASH_INPUT}")

    # Clean up temporary files
    if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/files_to_hash.txt")
        file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/files_to_hash.txt")
    endif()
    if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/untracked_files.txt")
        file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/untracked_files.txt")
    endif()

    # Return based on requested format
    if(ARG_HASH_FORMAT STREQUAL "short")
        # Return XOR-folded 8 character hash that incorporates entire hash
        hash_to_short("${FINAL_HASH}" SHORT_HASH)
        set(${OUTPUT_VAR} "${SHORT_HASH}" PARENT_SCOPE)
    elseif(ARG_HASH_FORMAT STREQUAL "full")
        # Return full SHA1 hash
        set(${OUTPUT_VAR} "${FINAL_HASH}" PARENT_SCOPE)
    else()
        # Default: Convert hash to version format (uses XOR-folded hash internally)
        hash_to_version("${FINAL_HASH}" VERSION_STRING)
        set(${OUTPUT_VAR} "${VERSION_STRING}" PARENT_SCOPE)
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
