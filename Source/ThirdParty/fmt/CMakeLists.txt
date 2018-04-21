include(CheckSymbolExists)
if (WIN32)
  check_symbol_exists(open io.h HAVE_OPEN)
else ()
  check_symbol_exists(open fcntl.h HAVE_OPEN)
endif ()
# Define the fmt library, its includes and the needed defines.
# *.cc are added to FMT_HEADERS for the header-only configuration.
set(FMT_HEADERS container.h format.h format.cc ostream.h ostream.cc printf.h
                printf.cc string.h time.h)
if (HAVE_OPEN)
  set(FMT_HEADERS ${FMT_HEADERS} posix.h)
  set(FMT_SOURCES ${FMT_SOURCES} posix.cc)
endif ()

add_library(fmt INTERFACE)
target_compile_definitions(fmt INTERFACE FMT_HEADER_ONLY=1 FMT_HAS_GXX_CXX11=1 FMT_USE_RVALUE_REFERENCES=1 FMT_USE_VARIADIC_TEMPLATES=1)
target_include_directories(fmt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
