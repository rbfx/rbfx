set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (Qt5Widgets_FOUND)
    file(GLOB_RECURSE SOURCE_FILES *.h *.cpp)
    add_executable(profiler_gui WIN32 ${SOURCE_FILES} $<TARGET_OBJECTS:easy_profiler> resources.qrc resources.rc)
    target_link_libraries(profiler_gui Qt5::Widgets)
    set_target_properties(profiler_gui PROPERTIES OUTPUT_NAME Profiler)
    install(TARGETS profiler_gui RUNTIME DESTINATION bin)
    if (TARGET easy_profiler_md)
        target_link_libraries(profiler_gui easy_profiler_md)
    else ()
        target_link_objects(profiler_gui easy_profiler)
    endif ()
    deploy_qt_dlls (profiler_gui)
    if (WIN32)
        target_compile_definitions(profiler_gui PRIVATE -D_WIN32_WINNT=0x0600)
    endif ()
    if (MINGW)
        target_compile_definitions(profiler_gui PRIVATE -DSTRSAFE_NO_DEPRECATE)
    endif ()
else ()
    message(WARNING
        "Qt5 not found! Profiler GUI application will not be built!\n"
        "You may have to specify path to Qt SDK in CMAKE_PREFIX_PATH variable."
    )
endif ()

