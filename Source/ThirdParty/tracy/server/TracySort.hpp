#ifndef __TRACYSORT_HPP__
#define __TRACYSORT_HPP__

#ifdef TRACY_NO_PARALLEL_ALGORITHMS // rbfx: Need a way to disable use of tbb. It is broken on any version of clang on ubuntu 18.04 (2020-02-20).
#  define NO_PARALLEL_SORT
#elif ( defined _MSC_VER && _MSVC_LANG >= 201703L ) || __cplusplus >= 201703L
#  if __has_include(<execution>)
#    include <algorithm>
#    include <execution>
#  else
#    define NO_PARALLEL_SORT
#  endif
#else
#  define NO_PARALLEL_SORT
#endif

#include "tracy_pdqsort.h"

#endif
