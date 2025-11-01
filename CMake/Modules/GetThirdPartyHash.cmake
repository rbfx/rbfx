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
# FNV-1a hash function - processes hex string (SHA1 hash)
function(fnv1a_hash input output)
  set(_hash "2166136261")  # FNV offset basis (32-bit)
  string(LENGTH "${input}" _len)

  # Process input as hex digits (2 chars = 1 byte)
  math(EXPR _num_bytes "${_len} / 2")
  math(EXPR _last_byte "${_num_bytes} - 1")

  foreach(_i RANGE 0 ${_last_byte})
    math(EXPR _pos "${_i} * 2")
    string(SUBSTRING "${input}" ${_pos} 2 _hex_byte)
    # Convert hex string to decimal
    math(EXPR _byte "0x${_hex_byte}" OUTPUT_FORMAT DECIMAL)
    math(EXPR _hash "(${_hash} ^ ${_byte}) * 16777619")
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
    set(MODIFIED_SET "")
    if(MODIFIED_FILES)
        string(REPLACE "\n" ";" MODIFIED_LINES "${MODIFIED_FILES}")
        foreach(MOD_FILE ${MODIFIED_LINES})
            if(NOT MOD_FILE STREQUAL "")
                set(MODIFIED_SET_${MOD_FILE} TRUE)
            endif()
        endforeach()
    endif()

    # Collect files that need hashing (modified + deleted)
    set(FILES_TO_HASH "")
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

                set(FULL_FILE_PATH "${GIT_ROOT}/${FILE_PATH}")

                # Check if file is modified or deleted
                if(NOT EXISTS "${FULL_FILE_PATH}")
                    # File was deleted locally
                    list(APPEND FILE_HASH_LIST "${FILE_MODE} deleted ${STAGE}\t${FILE_PATH}")
                elseif(DEFINED MODIFIED_SET_${FILE_PATH})
                    # File is modified, collect it for batch hashing
                    list(APPEND FILES_TO_HASH "${FULL_FILE_PATH}")
                    list(APPEND FILE_HASH_LIST "${FILE_MODE} PLACEHOLDER_${FILE_PATH} ${STAGE}\t${FILE_PATH}")
                else()
                    # File unchanged, use index hash
                    list(APPEND FILE_HASH_LIST "${FILE_MODE} ${INDEX_HASH} ${STAGE}\t${FILE_PATH}")
                endif()
            endif()
        endif()
    endforeach()

    # Batch hash all modified files in one git command (huge performance win!)
    if(FILES_TO_HASH)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} hash-object --stdin-paths
            WORKING_DIRECTORY "${GIT_ROOT}"
            INPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/files_to_hash.txt"
            OUTPUT_VARIABLE BATCH_HASHES
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE HASH_RESULT
        )

        # Write files to a temp file for stdin-paths
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/files_to_hash.txt" "")
        foreach(FILE_PATH ${FILES_TO_HASH})
            file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/files_to_hash.txt" "${FILE_PATH}\n")
        endforeach()

        # Re-run with the file created
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
            # Replace placeholders with actual hashes
            string(REPLACE "\n" ";" HASH_LINES "${BATCH_HASHES}")
            list(LENGTH FILES_TO_HASH NUM_FILES)
            list(LENGTH HASH_LINES NUM_HASHES)

            if(NUM_FILES EQUAL NUM_HASHES)
                # Build a map of file->hash
                math(EXPR LAST_INDEX "${NUM_FILES} - 1")
                foreach(IDX RANGE 0 ${LAST_INDEX})
                    list(GET FILES_TO_HASH ${IDX} FILE_PATH)
                    list(GET HASH_LINES ${IDX} HASH_VALUE)
                    file(RELATIVE_PATH REL_FILE "${GIT_ROOT}" "${FILE_PATH}")
                    set(HASH_MAP_${REL_FILE} "${HASH_VALUE}")
                endforeach()

                # Update FILE_HASH_LIST with actual hashes
                set(UPDATED_LIST "")
                foreach(ENTRY ${FILE_HASH_LIST})
                    if(ENTRY MATCHES "PLACEHOLDER_(.+) ")
                        set(REL_FILE "${CMAKE_MATCH_1}")
                        if(DEFINED HASH_MAP_${REL_FILE})
                            string(REPLACE "PLACEHOLDER_${REL_FILE}" "${HASH_MAP_${REL_FILE}}" ENTRY "${ENTRY}")
                        endif()
                    endif()
                    list(APPEND UPDATED_LIST "${ENTRY}")
                endforeach()
                set(FILE_HASH_LIST "${UPDATED_LIST}")
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
        string(REPLACE "\n" ";" UNTRACKED_LINES "${UNTRACKED_OUTPUT}")
        foreach(UNTRACKED_PATH ${UNTRACKED_LINES})
            if(NOT UNTRACKED_PATH STREQUAL "")
                set(UNTRACKED_FULL_PATH "${GIT_ROOT}/${UNTRACKED_PATH}")
                if(EXISTS "${UNTRACKED_FULL_PATH}")
                    list(APPEND UNTRACKED_FILES "${UNTRACKED_FULL_PATH}")
                    list(APPEND FILE_HASH_LIST "untracked PLACEHOLDER_UNTRACKED_${UNTRACKED_PATH} 0\t${UNTRACKED_PATH}")
                endif()
            endif()
        endforeach()

        if(UNTRACKED_FILES)
            file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/untracked_files.txt" "")
            foreach(FILE_PATH ${UNTRACKED_FILES})
                file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/untracked_files.txt" "${FILE_PATH}\n")
            endforeach()

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
                        list(GET UNTRACKED_FILES ${IDX} FILE_PATH)
                        list(GET UNTRACKED_HASH_LINES ${IDX} HASH_VALUE)
                        file(RELATIVE_PATH REL_FILE "${GIT_ROOT}" "${FILE_PATH}")
                        set(UNTRACKED_HASH_MAP_${REL_FILE} "${HASH_VALUE}")
                    endforeach()

                    # Update FILE_HASH_LIST with actual hashes
                    set(UPDATED_LIST "")
                    foreach(ENTRY ${FILE_HASH_LIST})
                        if(ENTRY MATCHES "PLACEHOLDER_UNTRACKED_(.+) ")
                            set(REL_FILE "${CMAKE_MATCH_1}")
                            if(DEFINED UNTRACKED_HASH_MAP_${REL_FILE})
                                string(REPLACE "PLACEHOLDER_UNTRACKED_${REL_FILE}" "${UNTRACKED_HASH_MAP_${REL_FILE}}" ENTRY "${ENTRY}")
                            endif()
                        endif()
                        list(APPEND UPDATED_LIST "${ENTRY}")
                    endforeach()
                    set(FILE_HASH_LIST "${UPDATED_LIST}")
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
