cmake_minimum_required (VERSION 3.6)

if(TARGET gtest)
    add_subdirectory(TestFramework)
    add_subdirectory(GPUTestFramework)
    if(DILIGENT_BUILD_CORE_TESTS)
        add_subdirectory(DiligentCoreTest)
        add_subdirectory(DiligentCoreAPITest)
    endif()
endif()

if (DILIGENT_BUILD_CORE_INCLUDE_TEST)
    add_subdirectory(IncludeTest)
endif()
