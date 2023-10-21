cmake_minimum_required (VERSION 3.6)

find_package(PythonInterp 3 REQUIRED)
set(FILE2STRING_PATH "${CMAKE_CURRENT_SOURCE_DIR}/script.py" CACHE INTERNAL "File2String utility")
