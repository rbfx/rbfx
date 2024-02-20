cmake_minimum_required(VERSION 2.8.12)
project(libmysofa C CXX)

include(CheckCCompilerFlag)
include(GenerateExportHeader)
include(GNUInstallDirs)

option(BUILD_TESTS "Build test programs" ON)
option(BUILD_SHARED_LIBS "Build shared library" ON)
option(BUILD_STATIC_LIBS "Build static library" ON)
option(CODE_COVERAGE "Enable coverage reporting" OFF)
option(ADDRESS_SANITIZE "Enable address sanitizer" OFF)

set(CPACK_PACKAGE_DESCRIPTION "SOFA file reader for better HRTFs")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "This library reads AES SOFA files and normalizes them")
set(CPACK_PACKAGE_VENDOR "Symonics GmbH")
set(CPACK_PACKAGE_CONTACT "umlaeute@debian.org")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "IOhannes m zmölnig")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "3")
set(CPACK_PACKAGE_VERSION_PATCH "2")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "zlib1g")

set(PKG_CONFIG_PRIVATELIBS "")

set(PROJECT_VERSION
    "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")

check_c_compiler_flag(-Wall C_HAS_WALL)
if(C_HAS_WALL)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif(C_HAS_WALL)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -DVDEBUG")

if(${ADDRESS_SANITIZE} STREQUAL "ON")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -O1 -fno-omit-frame-pointer -g")
endif()

install(FILES share/default.sofa DESTINATION ${CMAKE_INSTALL_DATADIR}/libmysofa)
install(FILES share/MIT_KEMAR_normal_pinna.sofa
        DESTINATION ${CMAKE_INSTALL_DATADIR}/libmysofa)

if(BUILD_TESTS)

  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
  include(FindCUnit)
  include_directories(${CUNIT_INCLUDE_DIRS})

  find_package(CUnit REQUIRED cunit)

  enable_testing()

  add_test(Mesh2HRTF src/mysofa2json -c
           ${PROJECT_SOURCE_DIR}/tests/Mesh2HRTF.sofa)
  add_test(latTestAziBeRTA_Resamp1_Fran src/mysofa2json -c
           ${PROJECT_SOURCE_DIR}/tests/latTestAziBeRTA_Resamp1_Fran.sofa)
  add_test(D1_48K_24bit_0.3s_FIR_SOFA src/mysofa2json
           ${PROJECT_SOURCE_DIR}/tests/D1_48K_24bit_0.3s_FIR_SOFA.sofa)
  add_test(H20_44K_16bit_256tap_FIR_SOFA ${PROJECT_SOURCE_DIR}/tests/compareIgnoreNew.sh
           ${PROJECT_SOURCE_DIR}/tests/H20_44K_16bit_256tap_FIR_SOFA)
  add_test(MIT_KEMAR_large_pinna ${PROJECT_SOURCE_DIR}/tests/compare.sh
           ${PROJECT_SOURCE_DIR}/tests/MIT_KEMAR_large_pinna)
  add_test(MIT_KEMAR_normal_pinna ${PROJECT_SOURCE_DIR}/tests/compareIgnoreNew.sh
           ${PROJECT_SOURCE_DIR}/tests/MIT_KEMAR_normal_pinna)
  add_test(MIT_KEMAR_normal_pinna.old ${PROJECT_SOURCE_DIR}/tests/compare.sh
           ${PROJECT_SOURCE_DIR}/tests/MIT_KEMAR_normal_pinna.old)
  add_test(dtf_nh2 ${PROJECT_SOURCE_DIR}/tests/compareIgnoreNew.sh
           ${PROJECT_SOURCE_DIR}/tests/dtf_nh2)
  add_test(hrtf_c_nh898 ${PROJECT_SOURCE_DIR}/tests/compareIgnoreNew.sh
           ${PROJECT_SOURCE_DIR}/tests/hrtf_c_nh898)
  foreach(
    ISSUE
    72
    75
    76
    77
    79
    82
    83
    84
    130
    132
    135
    136
    137
    138
    156
    163
    167a
    167b
    168
    171
    173
)
    # issues with osx    96)
    add_test(fail-issue-${ISSUE} ${PROJECT_SOURCE_DIR}/tests/notcrashed.sh
             ${PROJECT_SOURCE_DIR}/tests/fail-issue-${ISSUE})
  endforeach(ISSUE)
  add_test(CIPIC_subject_003_hrir_final ${PROJECT_SOURCE_DIR}/tests/compare.sh
           ${PROJECT_SOURCE_DIR}/tests/CIPIC_subject_003_hrir_final)
  add_test(FHK_HRIR_L2354 ${PROJECT_SOURCE_DIR}/tests/compare.sh ${PROJECT_SOURCE_DIR}/tests/FHK_HRIR_L2354)
  add_test(LISTEN_1002_IRC_1002_C_HRIR ${PROJECT_SOURCE_DIR}/tests/compare.sh
           ${PROJECT_SOURCE_DIR}/tests/LISTEN_1002_IRC_1002_C_HRIR)
  add_test(Pulse ${PROJECT_SOURCE_DIR}/tests/compare.sh ${PROJECT_SOURCE_DIR}/tests/Pulse)
  add_test(Tester ${PROJECT_SOURCE_DIR}/tests/compare.sh ${PROJECT_SOURCE_DIR}/tests/tester)
  add_test(TU-Berlin_QU_KEMAR_anechoic_radius_0.5_1_2_3_m ${PROJECT_SOURCE_DIR}/tests/compare.sh
           ${PROJECT_SOURCE_DIR}/tests/TU-Berlin_QU_KEMAR_anechoic_radius_0.5_1_2_3_m)
  add_test(TU-Berlin_QU_KEMAR_anechoic_radius_0.5m ${PROJECT_SOURCE_DIR}/tests/compare.sh
           ${PROJECT_SOURCE_DIR}/tests/TU-Berlin_QU_KEMAR_anechoic_radius_0.5m)
  add_test(example_dummy_sofa48 ${PROJECT_SOURCE_DIR}/tests/compare.sh
           ${PROJECT_SOURCE_DIR}/tests/example_dummy_sofa48)
  add_test(TestSOFA48_netcdf472 ${PROJECT_SOURCE_DIR}/tests/compare.sh
           ${PROJECT_SOURCE_DIR}/tests/TestSOFA48_netcdf472)
  add_test(example_dummy_sofa48_with_user_defined_variable ${PROJECT_SOURCE_DIR}/tests/compare.sh
           ${PROJECT_SOURCE_DIR}/tests/example_dummy_sofa48_with_user_defined_variable)
endif(BUILD_TESTS)

add_subdirectory(src)

include(CPack)

# Install exported targets as config files for use from install tree
install(EXPORT mysofa_exports
  NAMESPACE mysofa::
  FILE mysofaTargets.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mysofa
)

# Create a target export file for use from the build tree.
export(EXPORT mysofa_exports
  NAMESPACE mysofa::
  FILE ${CMAKE_CURRENT_BINARY_DIR}/mysofaTargets.cmake
)
