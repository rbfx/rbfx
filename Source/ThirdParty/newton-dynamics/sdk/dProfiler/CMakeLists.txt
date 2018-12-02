# Copyright (c) <2014-2017> <Newton Game Dynamics>
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely.

cmake_minimum_required(VERSION 3.10.0)

if (MSVC)

    include_directories(dTimeTracker/)
    include_directories(../dMath/)
    include_directories(../dContainers/)

    #include_directories(dTimeTrackerViewer)

    add_subdirectory(dTimeTracker)
    #add_subdirectory(dTimeTrackerViewer)

endif (MSVC)
