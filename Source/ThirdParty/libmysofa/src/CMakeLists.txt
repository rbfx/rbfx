macro(use_c99)
  if(CMAKE_VERSION VERSION_LESS "3.1")
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set(CMAKE_C_FLAGS "--std=gnu99 -pedantic ${CMAKE_C_FLAGS}")
    endif()
  else()
    set(CMAKE_C_STANDARD 99)
  endif()
endmacro(use_c99)

use_c99()

configure_file(config.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(NOT MSVC)
  if(NOT WIN32)
    find_library(MATH m)
  else()
    set(MATH "")
  endif()
  include(FindZLIB)
else()
  set(MATH "")
  find_program(NUGET nuget)
  if(NUGET)
    execute_process(COMMAND ${NUGET} install zlib)
  endif()
  include_directories(
    ${PROJECT_SOURCE_DIR}/windows/third-party/zlib-1.2.11/include/)
endif()

if(NOT MSVC)
  if(NOT WIN32)
    find_library(MATH m)
  else()
    set(MATH "")
  endif()
  include(FindZLIB)
  include_directories(${ZLIB_INCLUDE_DIRS})
  set(PKG_CONFIG_PRIVATELIBS "-lm ${PKG_CONFIG_PRIVATELIBS}")
  set(PKG_CONFIG_PRIVATELIBS "-lz ${PKG_CONFIG_PRIVATELIBS}")
else()
  set(MATH "")
  find_program(NUGET nuget)
  if(NOT NUGET)
    message(
      FATAL
      "Cannot find nuget command line tool.\nInstall it with e.g. choco install nuget.commandline"
    )
  else()
    execute_process(COMMAND ${NUGET} install zlib)
  endif()
  include_directories(
    ${PROJECT_SOURCE_DIR}/windows/third-party/zlib-1.2.11/include/)
endif()

set(libsrc
    hrtf/reader.c
    hdf/superblock.c
    hdf/dataobject.c
    hdf/btree.c
    hdf/fractalhead.c
    hdf/gunzip.c
    hdf/gcol.c
    hrtf/check.c
    hrtf/spherical.c
    hrtf/lookup.c
    hrtf/tools.c
    hrtf/kdtree.c
    hrtf/neighbors.c
    hrtf/interpolate.c
    hrtf/resample.c
    hrtf/loudness.c
    hrtf/minphase.c
    hrtf/easy.c
    hrtf/cache.c
    resampler/speex_resampler.c)

set(public-headers
    hrtf/mysofa.h
)

if(BUILD_STATIC_LIBS)
add_library(mysofa-static STATIC ${libsrc})
target_include_directories(mysofa-static
  PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/hrtf>"
  $<INSTALL_INTERFACE:include/>
)
target_link_libraries(mysofa-static LINK_PRIVATE ${MATH} ${ZLIB_LIBRARIES})
set_target_properties(
  mysofa-static
  PROPERTIES OUTPUT_NAME mysofa CLEAN_DIRECT_OUTPUT 1 POSITION_INDEPENDENT_CODE
                                                      ${BUILD_SHARED_LIBS})
set_target_properties( mysofa-static PROPERTIES PUBLIC_HEADER "${public-headers}" )
install(TARGETS mysofa-static
  EXPORT mysofa_exports
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  if(UNIX
     AND CODE_COVERAGE
     AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(mysofa-static PUBLIC -g -O0 -Wall -fprofile-arcs
                                                -ftest-coverage)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
      target_link_options(mysofa-static PUBLIC --coverage)
    else()
      target_link_libraries(mysofa-static LINK_PUBLIC gcov --coverage)
    endif()
  endif()
endif()

if(BUILD_SHARED_LIBS)
  add_library(mysofa-shared SHARED ${libsrc})
  target_include_directories(mysofa-shared
    PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/hrtf>"
    $<INSTALL_INTERFACE:include/>
  )
  target_link_libraries(mysofa-shared PRIVATE ${MATH} ${ZLIB_LIBRARIES})
  set_target_properties(mysofa-shared
                        PROPERTIES OUTPUT_NAME mysofa CLEAN_DIRECT_OUTPUT 1)
  # Rename the Windows import library (stub) of the DLL to prevent a name clash
  # with the mysofa-static library.
  set_target_properties(mysofa-shared
                        PROPERTIES ARCHIVE_OUTPUT_NAME mysofa_shared)

  set_property(
    TARGET mysofa-shared
    PROPERTY
      VERSION
      "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}"
  )
  set_property(TARGET mysofa-shared PROPERTY SOVERSION
                                             ${CPACK_PACKAGE_VERSION_MAJOR})
  set_property(TARGET mysofa-shared PROPERTY C_VISIBILITY_PRESET hidden)
  generate_export_header(mysofa-shared
    BASE_NAME MYSOFA
    EXPORT_FILE_NAME mysofa_export.h
    EXPORT_MACRO_NAME MYSOFA_EXPORT
    STATIC_DEFINE MYSOFA_STATIC
  )

  if(UNIX
     AND CODE_COVERAGE
     AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(mysofa-shared PUBLIC -g -O0 -Wall -fprofile-arcs
                                                -ftest-coverage)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
      target_link_options(mysofa-shared PUBLIC --coverage)
    else()
      target_link_libraries(mysofa-shared LINK_PUBLIC gcov --coverage)
    endif()
  endif()

  install(
    TARGETS mysofa-shared
    EXPORT mysofa_exports
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
else()
  generate_export_header(mysofa-static BASE_NAME mysofa EXPORT_FILE_NAME
                         mysofa_export.h)
endif()

if(BUILD_TESTS)
  add_executable(mysofa2json tests/sofa2json.c tests/json.c)
  if(BUILD_STATIC_LIBS)
    target_link_libraries(mysofa2json mysofa-static)
  else()
    target_link_libraries(mysofa2json mysofa-shared)
  endif()

  add_executable(
    external
    tests/external.c
    tests/check.c
    tests/check_data.c
    tests/lookup.c
    tests/neighbors.c
    tests/interpolate.c
    tests/resample.c
    tests/loudness.c
    tests/minphase.c
    tests/easy.c
    tests/cache.c
    tests/json.c
    tests/user_defined_variable.c)
  if(BUILD_STATIC_LIBS)
    target_link_libraries(external mysofa-static ${CUNIT_LIBRARIES})
  else()
    target_link_libraries(external mysofa-shared ${CUNIT_LIBRARIES} m)
  endif()
  add_test(
    NAME external
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMAND external)

  add_executable(internal tests/internal.c tests/tools.c hrtf/tools.c)
  target_link_libraries(internal m ${CUNIT_LIBRARIES})
  add_test(
    NAME internal
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMAND internal)

  add_executable(multithread tests/multithread.c)
  if(BUILD_STATIC_LIBS)
    target_link_libraries(multithread mysofa-static pthread)
  else()
    target_link_libraries(multithread mysofa-shared pthread m)
  endif()
  add_test(
    NAME multithread
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMAND multithread)

  if(UNIX
     AND CODE_COVERAGE
     AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(internal PUBLIC -g -O0 -Wall -fprofile-arcs
                                           -ftest-coverage)
    target_link_libraries(internal gcov --coverage)
  endif()

  install(
    TARGETS mysofa2json
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(BUILD_TESTS)

set(PKG_CONFIG_PRIVATELIBS
    "${PKG_CONFIG_PRIVATELIBS}"
    PARENT_SCOPE)
