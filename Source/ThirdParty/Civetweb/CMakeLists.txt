#
# Copyright (c) 2008-2020 the Urho3D project.
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

file (GLOB_RECURSE SOURCE_FILES *.c *.h *.inl)
add_library (Civetweb STATIC ${SOURCE_FILES})
target_include_directories(Civetweb
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/Civetweb>
)

# Define preprocessor macros
# Always disable Common Gateway Interface
target_compile_definitions(Civetweb PRIVATE -DNO_CGI -DNO_FILES -DNO_CACHING)
if (WIN32)
    include (CheckStructHasMember)
    check_struct_has_member (struct\ timespec tv_sec time.h HAVE_STRUCT_TIMESPEC_TV_SEC)
    if (HAVE_STRUCT_TIMESPEC_TV_SEC)
        include (CheckSymbolExists)
        check_symbol_exists (_TIMESPEC_DEFINED time.h HAVE__TIMESPEC_DEFINED)
        if (NOT HAVE__TIMESPEC_DEFINED)
            target_compile_definitions(Civetweb PRIVATE -D_TIMESPEC_DEFINED)
        endif ()
    endif ()
    target_link_libraries(Civetweb ws2_32.lib)
elseif (APPLE AND (NOT IOS OR IPHONEOS_DEPLOYMENT_TARGET STREQUAL "" OR NOT IPHONEOS_DEPLOYMENT_TARGET VERSION_LESS 10.0))    # We cheat a little bit here with a hard-coded check because iOS 10.3 SDK when targeting 9.3 has strong symbol for iPhoneOS archs but weak symbol for iPhoneSimulator archs
    include (CheckLibraryExists)
    check_library_exists (c clock_gettime "" FOUND_CLOCK_GETTIME_IN_C)
    if (FOUND_CLOCK_GETTIME_IN_C)
        target_compile_definitions(Civetweb PRIVATE -DHAVE_CLOCK_GETTIME)
    endif ()
endif ()

if (URHO3D_SSL)
    target_compile_definitions(Civetweb PRIVATE -DNO_SSL_DL)
    if (NOT BUILD_SHARED_LIBS)
        set (OPENSSL_USE_STATIC_LIBS ON)
    endif ()
    if (MSVC AND URHO3D_STATIC_RUNTIME)
        set (OPENSSL_MSVC_STATIC_RT ON)
    endif ()
    if (NOT DEFINED OPENSSL_ROOT_DIR AND APPLE)
        set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
    endif ()
    find_package(OpenSSL REQUIRED)
    target_link_libraries(Civetweb PRIVATE ${OPENSSL_LIBRARIES})
    target_include_directories(Civetweb PRIVATE ${OPENSSL_INCLUDE_DIR})
else ()
    target_compile_definitions(Civetweb PRIVATE -DNO_SSL)
endif ()

# TODO: This needs a better detection.
target_compile_definitions(Civetweb PRIVATE -DHAVE_POLL=1)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(Civetweb PRIVATE -DCIVETWEB_DLL_EXPORTS PUBLIC -DCIVETWEB_DLL_IMPORTS)
endif ()

install (DIRECTORY include/ DESTINATION ${DEST_THIRDPARTY_HEADERS_DIR}/ FILES_MATCHING PATTERN *.h)

if (NOT URHO3D_MERGE_STATIC_LIBS)
    install(TARGETS Civetweb EXPORT Urho3D ARCHIVE DESTINATION ${DEST_ARCHIVE_DIR_CONFIG})
endif ()
