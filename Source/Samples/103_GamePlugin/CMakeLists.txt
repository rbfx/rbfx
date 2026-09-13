# Copyright (c) 2017-2026 the rbfx project.
# This work is licensed under the terms of the MIT license.
# For a copy, see <https:#opensource.org/licenses/MIT> or the accompanying LICENSE file.

file (GLOB SOURCE_FILES *.cpp *.h)
add_plugin (Sample.103_GamePlugin "${SOURCE_FILES}")

install(TARGETS Sample.103_GamePlugin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    BUNDLE  DESTINATION ${CMAKE_INSTALL_BINDIR}
)
