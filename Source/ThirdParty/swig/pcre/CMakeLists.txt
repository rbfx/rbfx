# CMakeLists.txt
#
#
# This file allows building PCRE with the CMake configuration and build
# tool. Download CMake in source or binary form from http://www.cmake.org/
#
# Original listfile by Christian Ehrlicher <Ch.Ehrlicher@gmx.de>
# Refined and expanded by Daniel Richard G. <skunk@iSKUNK.ORG>
# 2007-09-14 mod by Sheri so 7.4 supported configuration options can be entered
# 2007-09-19 Adjusted by PH to retain previous default settings
# 2007-12-26 (a) On UNIX, use names libpcre instead of just pcre
#            (b) Ensure pcretest and pcregrep link with the local library,
#                not a previously-installed one.
#            (c) Add PCRE_SUPPORT_LIBREADLINE, PCRE_SUPPORT_LIBZ, and
#                PCRE_SUPPORT_LIBBZ2.
# 2008-01-20 Brought up to date to include several new features by Christian
#            Ehrlicher.
# 2008-01-22 Sheri added options for backward compatibility of library names
#            when building with minGW:
#            if "ON", NON_STANDARD_LIB_PREFIX causes shared libraries to
#            be built without "lib" as prefix. (The libraries will be named
#            pcre.dll, pcreposix.dll and pcrecpp.dll).
#            if "ON", NON_STANDARD_LIB_SUFFIX causes shared libraries to
#            be built with suffix of "-0.dll". (The libraries will be named
#            libpcre-0.dll, libpcreposix-0.dll and libpcrecpp-0.dll - same names
#            built by default with Configure and Make.
# 2008-01-23 PH removed the automatic build of pcredemo.
# 2008-04-22 PH modified READLINE support so it finds NCURSES when needed.
# 2008-07-03 PH updated for revised UCP property support (change of files)
# 2009-03-23 PH applied Steven Van Ingelgem's patch to change the name
#            CMAKE_BINARY_DIR to PROJECT_BINARY_DIR so that it works when PCRE
#            is included within another project.
# 2009-03-23 PH applied a modified version of Steven Van Ingelgem's patches to
#            add options to stop the building of pcregrep and the tests, and
#            to disable the final configuration report.
# 2009-04-11 PH applied Christian Ehrlicher's patch to show compiler flags that
#            are set by specifying a release type.
# 2010-01-02 PH added test for stdint.h
# 2010-03-02 PH added test for inttypes.h
# 2011-08-01 PH added PCREGREP_BUFSIZE
# 2011-08-22 PH added PCRE_SUPPORT_JIT
# 2011-09-06 PH modified WIN32 ADD_TEST line as suggested by Sergey Cherepanov
# 2011-09-06 PH added PCRE_SUPPORT_PCREGREP_JIT
# 2011-10-04 Sheri added support for including coff data in windows shared libraries
#            compiled with MINGW if pcre.rc and/or pcreposix.rc are placed in
#            the source dir by the user prior to building
# 2011-10-04 Sheri changed various add_test's to use exes' location built instead
#            of DEBUG location only (likely only matters in MSVC)
# 2011-10-04 Sheri added scripts to provide needed variables to RunTest and
#            RunGrepTest (used for UNIX and Msys)
# 2011-10-04 Sheri added scripts to provide needed variables and to execute
#            RunTest.bat in Win32 (for effortless testing with "make test")
# 2011-10-04 Sheri Increased minimum required cmake version
# 2012-01-06 PH removed pcre_info.c and added pcre_string_utils.c
# 2012-01-10 Zoltan Herczeg added libpcre16 support
# 2012-01-13 Stephen Kelly added out of source build support
# 2012-01-17 PH applied Stephen Kelly's patch to parse the version data out
#            of the configure.ac file
# 2012-02-26 PH added support for libedit
# 2012-09-06 PH added support for PCRE_EBCDIC_NL25
# 2012-09-08 ChPe added PCRE32 support
# 2012-10-23 PH added support for VALGRIND and GCOV
# 2012-12-08 PH added patch from Daniel Richard G to quash some MSVC warnings
# 2013-07-01 PH realized that the "support" for GCOV was a total nonsense and
#            so it has been removed.
# 2013-10-08 PH got rid of the "source" command, which is a bash-ism (use ".")
# 2013-11-05 PH added support for PARENS_NEST_LIMIT
# 2016-03-01 PH applied Chris Wilson's patch for MSVC static build
# 2016-06-24 PH applied Chris Wilson's revised patch (adds a separate option)

PROJECT(PCRE C CXX)

#CMAKE_MINIMUM_REQUIRED(VERSION 2.8.0)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake) # for FindReadline.cmake

# Configuration checks

INCLUDE(CheckIncludeFile)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckTypeSize)

CHECK_INCLUDE_FILE(dirent.h     HAVE_DIRENT_H)
CHECK_INCLUDE_FILE(stdint.h     HAVE_STDINT_H)
CHECK_INCLUDE_FILE(inttypes.h   HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE(sys/stat.h   HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE(sys/types.h  HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE(unistd.h     HAVE_UNISTD_H)
CHECK_INCLUDE_FILE(windows.h    HAVE_WINDOWS_H)

CHECK_INCLUDE_FILE_CXX(type_traits.h            HAVE_TYPE_TRAITS_H)
CHECK_INCLUDE_FILE_CXX(bits/type_traits.h       HAVE_BITS_TYPE_TRAITS_H)

CHECK_FUNCTION_EXISTS(bcopy     HAVE_BCOPY)
CHECK_FUNCTION_EXISTS(memmove   HAVE_MEMMOVE)
CHECK_FUNCTION_EXISTS(strerror  HAVE_STRERROR)
CHECK_FUNCTION_EXISTS(strtoll   HAVE_STRTOLL)
CHECK_FUNCTION_EXISTS(strtoq    HAVE_STRTOQ)
CHECK_FUNCTION_EXISTS(_strtoi64 HAVE__STRTOI64)

CHECK_TYPE_SIZE("long long"             LONG_LONG)
CHECK_TYPE_SIZE("unsigned long long"    UNSIGNED_LONG_LONG)

# User-configurable options
#
# (Note: CMakeSetup displays these in alphabetical order, regardless of
# the order we use here)

SET(BUILD_SHARED_LIBS OFF CACHE BOOL
    "Build shared libraries instead of static ones.")

OPTION(PCRE_BUILD_PCRE8 "Build 8 bit PCRE library" ON)

OPTION(PCRE_BUILD_PCRE16 "Build 16 bit PCRE library" OFF)

OPTION(PCRE_BUILD_PCRE32 "Build 32 bit PCRE library" OFF)

SET(PCRE_EBCDIC OFF CACHE BOOL
    "Use EBCDIC coding instead of ASCII. (This is rarely used outside of mainframe systems.)")

SET(PCRE_EBCDIC_NL25 OFF CACHE BOOL
    "Use 0x25 as EBCDIC NL character instead of 0x15; implies EBCDIC.")

SET(PCRE_LINK_SIZE "2" CACHE STRING
    "Internal link size (2, 3 or 4 allowed). See LINK_SIZE in config.h.in for details.")

SET(PCRE_PARENS_NEST_LIMIT "250" CACHE STRING
    "Default nested parentheses limit. See PARENS_NEST_LIMIT in config.h.in for details.")

SET(PCRE_MATCH_LIMIT "10000000" CACHE STRING
    "Default limit on internal looping. See MATCH_LIMIT in config.h.in for details.")

SET(PCRE_MATCH_LIMIT_RECURSION "MATCH_LIMIT" CACHE STRING
    "Default limit on internal recursion. See MATCH_LIMIT_RECURSION in config.h.in for details.")

SET(PCREGREP_BUFSIZE "20480" CACHE STRING
    "Buffer size parameter for pcregrep. See PCREGREP_BUFSIZE in config.h.in for details.")

SET(PCRE_NEWLINE "LF" CACHE STRING
    "What to recognize as a newline (one of CR, LF, CRLF, ANY, ANYCRLF).")

SET(PCRE_NO_RECURSE OFF CACHE BOOL
    "If ON, then don't use stack recursion when matching. See NO_RECURSE in config.h.in for details.")

SET(PCRE_POSIX_MALLOC_THRESHOLD "10" CACHE STRING
    "Threshold for malloc() usage. See POSIX_MALLOC_THRESHOLD in config.h.in for details.")

SET(PCRE_SUPPORT_JIT OFF CACHE BOOL
    "Enable support for Just-in-time compiling.")

SET(PCRE_SUPPORT_PCREGREP_JIT ON CACHE BOOL
    "Enable use of Just-in-time compiling in pcregrep.")

SET(PCRE_SUPPORT_UTF OFF CACHE BOOL
    "Enable support for Unicode Transformation Format (UTF-8/UTF-16/UTF-32) encoding.")

SET(PCRE_SUPPORT_UNICODE_PROPERTIES OFF CACHE BOOL
    "Enable support for Unicode properties (if set, UTF support will be enabled as well).")

SET(PCRE_SUPPORT_BSR_ANYCRLF OFF CACHE BOOL
    "ON=Backslash-R matches only LF CR and CRLF, OFF=Backslash-R matches all Unicode Linebreaks")

SET(PCRE_SUPPORT_VALGRIND OFF CACHE BOOL
    "Enable Valgrind support.")

# Prepare build configuration

SET(pcre_have_type_traits 0)
SET(pcre_have_bits_type_traits 0)

IF(HAVE_TYPE_TRAITS_H)
        SET(pcre_have_type_traits 1)
ENDIF(HAVE_TYPE_TRAITS_H)

IF(HAVE_BITS_TYPE_TRAITS_H)
        SET(pcre_have_bits_type_traits 1)
ENDIF(HAVE_BITS_TYPE_TRAITS_H)

SET(pcre_have_long_long 0)
SET(pcre_have_ulong_long 0)

IF(HAVE_LONG_LONG)
        SET(pcre_have_long_long 1)
ENDIF(HAVE_LONG_LONG)

IF(HAVE_UNSIGNED_LONG_LONG)
        SET(pcre_have_ulong_long 1)
ENDIF(HAVE_UNSIGNED_LONG_LONG)

SET(PCRE_STATIC 1)

IF(NOT PCRE_BUILD_PCRE8 AND NOT PCRE_BUILD_PCRE16 AND NOT PCRE_BUILD_PCRE32)
        MESSAGE(FATAL_ERROR "At least one of PCRE_BUILD_PCRE8, PCRE_BUILD_PCRE16 or PCRE_BUILD_PCRE32 must be enabled")
ENDIF(NOT PCRE_BUILD_PCRE8 AND NOT PCRE_BUILD_PCRE16 AND NOT PCRE_BUILD_PCRE32)

IF(PCRE_BUILD_PCRE8)
        SET(SUPPORT_PCRE8 1)
ENDIF(PCRE_BUILD_PCRE8)

IF(PCRE_BUILD_PCRE16)
        SET(SUPPORT_PCRE16 1)
ENDIF(PCRE_BUILD_PCRE16)

IF(PCRE_BUILD_PCRE32)
        SET(SUPPORT_PCRE32 1)
ENDIF(PCRE_BUILD_PCRE32)

IF(PCRE_BUILD_PCREGREP AND NOT PCRE_BUILD_PCRE8)
        MESSAGE(STATUS "** PCRE_BUILD_PCRE8 must be enabled for the pcregrep program")
        SET(PCRE_BUILD_PCREGREP OFF)
ENDIF(PCRE_BUILD_PCREGREP AND NOT PCRE_BUILD_PCRE8)

IF(PCRE_SUPPORT_BSR_ANYCRLF)
        SET(BSR_ANYCRLF 1)
ENDIF(PCRE_SUPPORT_BSR_ANYCRLF)

IF(PCRE_SUPPORT_UTF OR PCRE_SUPPORT_UNICODE_PROPERTIES)
        SET(SUPPORT_UTF 1)
        SET(PCRE_SUPPORT_UTF ON)
ENDIF(PCRE_SUPPORT_UTF OR PCRE_SUPPORT_UNICODE_PROPERTIES)

IF(PCRE_SUPPORT_UNICODE_PROPERTIES)
        SET(SUPPORT_UCP 1)
ENDIF(PCRE_SUPPORT_UNICODE_PROPERTIES)

IF(PCRE_SUPPORT_JIT)
        SET(SUPPORT_JIT 1)
ENDIF(PCRE_SUPPORT_JIT)

IF(PCRE_SUPPORT_PCREGREP_JIT)
        SET(SUPPORT_PCREGREP_JIT 1)
ENDIF(PCRE_SUPPORT_PCREGREP_JIT)

IF(PCRE_SUPPORT_VALGRIND)
        SET(SUPPORT_VALGRIND 1)
ENDIF(PCRE_SUPPORT_VALGRIND)

# This next one used to contain
#       SET(PCRETEST_LIBS ${READLINE_LIBRARY})
# but I was advised to add the NCURSES test as well, along with
# some modifications to cmake/FindReadline.cmake which should
# make it possible to override the default if necessary. PH

# libedit is a plug-compatible alternative to libreadline

SET(NEWLINE "")

IF(PCRE_NEWLINE STREQUAL "LF")
        SET(NEWLINE "10")
ENDIF(PCRE_NEWLINE STREQUAL "LF")
IF(PCRE_NEWLINE STREQUAL "CR")
        SET(NEWLINE "13")
ENDIF(PCRE_NEWLINE STREQUAL "CR")
IF(PCRE_NEWLINE STREQUAL "CRLF")
        SET(NEWLINE "3338")
ENDIF(PCRE_NEWLINE STREQUAL "CRLF")
IF(PCRE_NEWLINE STREQUAL "ANY")
        SET(NEWLINE "-1")
ENDIF(PCRE_NEWLINE STREQUAL "ANY")
IF(PCRE_NEWLINE STREQUAL "ANYCRLF")
        SET(NEWLINE "-2")
ENDIF(PCRE_NEWLINE STREQUAL "ANYCRLF")

IF(NEWLINE STREQUAL "")
        MESSAGE(FATAL_ERROR "The PCRE_NEWLINE variable must be set to one of the following values: \"LF\", \"CR\", \"CRLF\", \"ANY\", \"ANYCRLF\".")
ENDIF(NEWLINE STREQUAL "")

IF(PCRE_EBCDIC)
        SET(EBCDIC 1)
IF(PCRE_NEWLINE STREQUAL "LF")
        SET(NEWLINE "21")
ENDIF(PCRE_NEWLINE STREQUAL "LF")
IF(PCRE_NEWLINE STREQUAL "CRLF")
        SET(NEWLINE "3349")
ENDIF(PCRE_NEWLINE STREQUAL "CRLF")
ENDIF(PCRE_EBCDIC)

IF(PCRE_EBCDIC_NL25)
        SET(EBCDIC 1)
        SET(EBCDIC_NL25 1)
IF(PCRE_NEWLINE STREQUAL "LF")
        SET(NEWLINE "37")
ENDIF(PCRE_NEWLINE STREQUAL "LF")
IF(PCRE_NEWLINE STREQUAL "CRLF")
        SET(NEWLINE "3365")
ENDIF(PCRE_NEWLINE STREQUAL "CRLF")
ENDIF(PCRE_EBCDIC_NL25)

IF(PCRE_NO_RECURSE)
        SET(NO_RECURSE 1)
ENDIF(PCRE_NO_RECURSE)

# Output files
CONFIGURE_FILE(config-cmake.h.in
               ${PROJECT_BINARY_DIR}/config.h
               @ONLY)

# Parse version numbers and date out of configure.ac

file(STRINGS ${PROJECT_SOURCE_DIR}/configure.ac
  configure_lines
  LIMIT_COUNT 50 # Read only the first 50 lines of the file
)

set(SEARCHED_VARIABLES "pcre_major" "pcre_minor" "pcre_prerelease" "pcre_date")
foreach(configure_line ${configure_lines})
    foreach(_substitution_variable ${SEARCHED_VARIABLES})
        string(TOUPPER ${_substitution_variable} _substitution_variable_upper)
        if (NOT ${_substitution_variable_upper})
            string(REGEX MATCH "m4_define\\(${_substitution_variable}, \\[(.*)\\]" MACTHED_STRING ${configure_line})
            if (CMAKE_MATCH_1)
                set(${_substitution_variable_upper} ${CMAKE_MATCH_1})
            endif()
        endif()
    endforeach()
endforeach()

CONFIGURE_FILE(pcre.h.in
               ${PROJECT_BINARY_DIR}/pcre.h
               @ONLY)

# What about pcre-config and libpcre.pc?

# Source code

SET(PCRE_HEADERS ${PROJECT_BINARY_DIR}/pcre.h)

IF(PCRE_BUILD_PCRE8)
SET(PCRE_SOURCES
  pcre_byte_order.c
  pcre_chartables.c
  pcre_compile.c
  pcre_config.c
  pcre_dfa_exec.c
  pcre_exec.c
  pcre_fullinfo.c
  pcre_get.c
  pcre_globals.c
  pcre_jit_compile.c
  pcre_maketables.c
  pcre_newline.c
  pcre_ord2utf8.c
  pcre_refcount.c
  pcre_string_utils.c
  pcre_study.c
  pcre_tables.c
  pcre_ucd.c
  pcre_valid_utf8.c
  pcre_version.c
  pcre_xclass.c
)

SET(PCREPOSIX_HEADERS pcreposix.h)

SET(PCREPOSIX_SOURCES pcreposix.c)

ENDIF(PCRE_BUILD_PCRE8)

IF(PCRE_BUILD_PCRE16)
SET(PCRE16_SOURCES
  pcre16_byte_order.c
  pcre16_chartables.c
  pcre16_compile.c
  pcre16_config.c
  pcre16_dfa_exec.c
  pcre16_exec.c
  pcre16_fullinfo.c
  pcre16_get.c
  pcre16_globals.c
  pcre16_jit_compile.c
  pcre16_maketables.c
  pcre16_newline.c
  pcre16_ord2utf16.c
  pcre16_refcount.c
  pcre16_string_utils.c
  pcre16_study.c
  pcre16_tables.c
  pcre16_ucd.c
  pcre16_utf16_utils.c
  pcre16_valid_utf16.c
  pcre16_version.c
  pcre16_xclass.c
)
ENDIF(PCRE_BUILD_PCRE16)

IF(PCRE_BUILD_PCRE32)
SET(PCRE32_SOURCES
  pcre32_byte_order.c
  pcre32_chartables.c
  pcre32_compile.c
  pcre32_config.c
  pcre32_dfa_exec.c
  pcre32_exec.c
  pcre32_fullinfo.c
  pcre32_get.c
  pcre32_globals.c
  pcre32_jit_compile.c
  pcre32_maketables.c
  pcre32_newline.c
  pcre32_ord2utf32.c
  pcre32_refcount.c
  pcre32_string_utils.c
  pcre32_study.c
  pcre32_tables.c
  pcre32_ucd.c
  pcre32_utf32_utils.c
  pcre32_valid_utf32.c
  pcre32_version.c
  pcre32_xclass.c
)
ENDIF(PCRE_BUILD_PCRE32)

IF(MINGW AND NOT PCRE_STATIC)
IF (EXISTS ${PROJECT_SOURCE_DIR}/pcre.rc)
ADD_CUSTOM_COMMAND(OUTPUT ${PROJECT_SOURCE_DIR}/pcre.o
PRE-LINK
COMMAND windres ARGS pcre.rc pcre.o
WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
COMMENT Using pcre coff info in mingw build)
SET(PCRE_SOURCES
  ${PCRE_SOURCES} ${PROJECT_SOURCE_DIR}/pcre.o
)
ENDIF(EXISTS ${PROJECT_SOURCE_DIR}/pcre.rc)
IF (EXISTS ${PROJECT_SOURCE_DIR}/pcreposix.rc)
ADD_CUSTOM_COMMAND(OUTPUT ${PROJECT_SOURCE_DIR}/pcreposix.o
PRE-LINK
COMMAND windres ARGS pcreposix.rc pcreposix.o
WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
COMMENT Using pcreposix coff info in mingw build)
SET(PCREPOSIX_SOURCES
  ${PCREPOSIX_SOURCES} ${PROJECT_SOURCE_DIR}/pcreposix.o
)
ENDIF(EXISTS ${PROJECT_SOURCE_DIR}/pcreposix.rc)
ENDIF(MINGW AND NOT PCRE_STATIC)

IF(MSVC AND NOT PCRE_STATIC)
IF (EXISTS ${PROJECT_SOURCE_DIR}/pcre.rc)
SET(PCRE_SOURCES
  ${PCRE_SOURCES} pcre.rc)
ENDIF(EXISTS ${PROJECT_SOURCE_DIR}/pcre.rc)
IF (EXISTS ${PROJECT_SOURCE_DIR}/pcreposix.rc)
SET(PCREPOSIX_SOURCES
  ${PCREPOSIX_SOURCES} pcreposix.rc)
ENDIF (EXISTS ${PROJECT_SOURCE_DIR}/pcreposix.rc)
ENDIF(MSVC AND NOT PCRE_STATIC)

# Build setup

ADD_DEFINITIONS(-DHAVE_CONFIG_H)

IF(MSVC)
        ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS)
ENDIF(MSVC)

SET(CMAKE_INCLUDE_CURRENT_DIR 1)
# needed to make sure to not link debug libs
# against release libs and vice versa
IF(WIN32)
  SET(CMAKE_DEBUG_POSTFIX "d")
ENDIF(WIN32)

SET(targets)

# Libraries
# pcre
IF(PCRE_BUILD_PCRE8)
ADD_LIBRARY(pcre STATIC ${PCRE_HEADERS} ${PCRE_SOURCES} ${PROJECT_BINARY_DIR}/config.h)
SET(targets ${targets} pcre)
ADD_LIBRARY(pcreposix STATIC ${PCREPOSIX_HEADERS} ${PCREPOSIX_SOURCES})
SET(targets ${targets} pcreposix)
TARGET_LINK_LIBRARIES(pcreposix pcre)
target_include_directories(pcre PUBLIC .)
if (PCRE_STATIC)
    target_compile_definitions(pcre PUBLIC -DPCRE_STATIC=1)
endif ()

IF(MINGW AND NOT PCRE_STATIC)
  IF(NON_STANDARD_LIB_PREFIX)
    SET_TARGET_PROPERTIES(pcre pcreposix
                        PROPERTIES PREFIX ""
    )
  ENDIF(NON_STANDARD_LIB_PREFIX)

  IF(NON_STANDARD_LIB_SUFFIX)
    SET_TARGET_PROPERTIES(pcre pcreposix
                        PROPERTIES SUFFIX "-0.dll"
    )
  ENDIF(NON_STANDARD_LIB_SUFFIX)
ENDIF(MINGW AND NOT PCRE_STATIC)

ENDIF(PCRE_BUILD_PCRE8)

IF(PCRE_BUILD_PCRE16)
ADD_LIBRARY(pcre16 ${PCRE_HEADERS} ${PCRE16_SOURCES} ${PROJECT_BINARY_DIR}/config.h)
SET(targets ${targets} pcre16)

IF(MINGW AND NOT PCRE_STATIC)
  IF(NON_STANDARD_LIB_PREFIX)
    SET_TARGET_PROPERTIES(pcre16
                        PROPERTIES PREFIX ""
    )
  ENDIF(NON_STANDARD_LIB_PREFIX)

  IF(NON_STANDARD_LIB_SUFFIX)
    SET_TARGET_PROPERTIES(pcre16
                        PROPERTIES SUFFIX "-0.dll"
    )
  ENDIF(NON_STANDARD_LIB_SUFFIX)
ENDIF(MINGW AND NOT PCRE_STATIC)

ENDIF(PCRE_BUILD_PCRE16)

IF(PCRE_BUILD_PCRE32)
ADD_LIBRARY(pcre32 ${PCRE_HEADERS} ${PCRE32_SOURCES} ${PROJECT_BINARY_DIR}/config.h)
SET(targets ${targets} pcre32)

IF(MINGW AND NOT PCRE_STATIC)
  IF(NON_STANDARD_LIB_PREFIX)
    SET_TARGET_PROPERTIES(pcre32
                        PROPERTIES PREFIX ""
    )
  ENDIF(NON_STANDARD_LIB_PREFIX)

  IF(NON_STANDARD_LIB_SUFFIX)
    SET_TARGET_PROPERTIES(pcre32
                        PROPERTIES SUFFIX "-0.dll"
    )
  ENDIF(NON_STANDARD_LIB_SUFFIX)
ENDIF(MINGW AND NOT PCRE_STATIC)

ENDIF(PCRE_BUILD_PCRE32)
