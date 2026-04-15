[![Standalone](https://github.com/GabTux/ppqsort_suite/actions/workflows/standalone.yml/badge.svg)](https://github.com/GabTux/ppqsort_suite/actions/workflows/standalone.yml)
[![Install](https://github.com/GabTux/ppqsort_suite/actions/workflows/install.yml/badge.svg)](https://github.com/GabTux/ppqsort_suite/actions/workflows/install.yml)
[![Tests](https://github.com/GabTux/ppqsort_suite/actions/workflows/tests.yml/badge.svg)](https://github.com/GabTux/ppqsort_suite/actions/workflows/tests.yml)
[![codecov](https://codecov.io/github/GabTux/PPQSort/graph/badge.svg?token=K7UVUZ4N1N)](https://codecov.io/github/GabTux/PPQSort)

# PPQSort (Parallel Pattern QuickSort)
Parallel Pattern Quicksort (PPQSort) is a **efficient implementation of parallel quicksort algorithm**, written by using **only** the C++20 features without using third party libraries (such as Intel TBB). PPQSort draws inspiration from [pdqsort](https://github.com/orlp/pdqsort), [BlockQuicksort](https://github.com/weissan/BlockQuicksort) and [cpp11sort](https://gitlab.com/daniel.langr/cpp11sort) and adds some further optimizations.

* **Focus on ease of use:** Using only C++20 features, header only implementation, user-friendly API.
* **Comprehensive test suite:** Ensures correctness and robustness through extensive testing. 
* **Benchmarks shows great performance:** Achieves impressive sorting times on various machines.

# Integration
PPQSort is header only implementation. All the files needed are in include directory.

Add to existing CMake project using [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake):
```CMake
include(cmake/CPM.cmake)
CPMAddPackage(
NAME PPQSort
GITHUB_REPOSITORY GabTux/PPQSort
VERSION 1.0.5 # change this to latest commit or release tag
)
target_link_libraries(YOUR_TARGET PPQSort::PPQSort)
```
Alternatively use FetchContent or just checkout the repository and add the include directory to the linker flags.

# Usage

PPQSort has similiar API as std::sort, you can use `ppqsort::execution::<policy>` policies to specify how the sort should run.
```cpp
// run parallel
ppqsort::sort(ppqsort::execution::par, input.begin(), input.end());

// Specify number of threads
ppqsort::sort(ppqsort::execution::par, input.begin(), input.end(), 16);

// provide custom comparator
ppqsort::sort(ppqsort::execution::par, input.begin(), input.end(), cmp);

// force branchless variant
ppqsort::sort(ppqsort::execution::par_force_branchless, input_str.begin(), input_str.end(), cmp);
```

PPQSort will by default use C++ threads, but if you prefer, you can link it with OpenMP and it will use OpenMP as a parallel backend. However you can still enforce C++ threads parallel backend even if linked with OpenMP:
```cpp
#define FORCE_CPP
#include <ppqsort.h>
// ... rest of the code ...
```

# Benchmark
We compared PPQSort with various parallel sorts. Benchmarks shows, that the PPQSort is one of the fastest parallel sorting algorithms across various input data and different machines.

| Name                            | Algorithm         | Memory usage | External dependencies                                                | Highlight                                           |
|---------------------------------|-------------------|--------------|----------------------------------------------------------------------|-----------------------------------------------------|
| PPQSort            | Quicksort         | in-place     | **None**                                                    | parallel pattern quicksort algorithm  |
| [GCC BQS](https://gcc.gnu.org/onlinedocs/libstdc++/manual/parallel_mode_design.html)            | Quicksort         | in-place     | OpenMP                                                    | allocating threads proportionally to subtask sizes  |
| [cpp11sort](https://gitlab.com/daniel.langr/cpp11sort)                       | Quicksort         | in-place     | **None**                                                                 | Header-only, C++11 compliant                        |
| oneTBB [parallel_sort](https://spec.oneapi.io/versions/latest/elements/oneTBB/source/algorithms/functions/parallel_sort_func.html#parallel-sort)          | quicksort         | out-place    | oneTBB                                                    | Splits input to small tasks                         |
| [poolSTL](https://github.com/alugowski/poolSTL) sort                         | Quicksort         | in-place     | **None**                                                                 | Header-only, C++17 compliant                        |
| Boost [block_indirect_sort](https://www.boost.org/doc/libs/develop/libs/sort/doc/html/sort/parallel.html#sort.parallel.block_indirect_sort)       | merging algorithm | out-place    | Boost                                                                | Upper bounded small memory usage                    |
| [AQsort](https://github.com/DanielLangr/AQsort)                          | Quicksort         | in-place     | OpenMP                                                    | Allows the sorting of multiple datasets at once     |
| [MPQsort](https://github.com/voronond/MPQsort/)              | Quicksort         | in-place     | OpenMP                                                    | Multiway Parallel Quicksort                         |
| [IPS4o](https://github.com/ips4o/ips4o)                        | Samplesort        | in-place     | oneTBB                                                    | Divides data into buckets and sort them recursively |

## Running on ARM cluster
* Fujitsu A64FX CPU
* NUMA architecture, 48 cores (4CPUs x 12cores)

Results for **INT**, input size was **2e9** (2 billions):

![arm_patterns_ext](https://github.com/GabTux/PPQSort/assets/24779546/95741ffe-d710-4360-afac-fa5dce3c50c1)

| Algorithm   | Random | Ascending | Descending | Rotated | OrganPipe | Heap | Total | Rank |
|-------------|----------------------------------|-------------------------------------|--------------------------------------|-----------------------------------|-------------------------------------|--------------------------------|---------------------------------|--------------------------------|
| PPQSort C++ | 5.84s                            | 1.84s                               | 4.55s                                | **1.38s**                    | **2.96s**                      | 5.58s                          | **22.15s**                          | **1**                              |
| GCC BQS     | 13.72s                           | 4.18s                               | 19.11s                               | 49.89s                            | 8.24s                               | 13.78s                         | 108.92s                         | 6                              |
| oneTBB      | 43.66s                           | **0.09s**                      | 8.62s                                | 13.84s                            | 8.12s                               | 43.9s                          | 118.23s                         | 9                              |
| poolSTL     | 34.63s                           | 5.61s                               | 7.23s                                | 14.78s                            | 7.81s                               | 46.88s                         | 116.94s                         | 7                              |
| MPQsort     | 13.35s                           | 5.74s                               | 5.77s                                | 4.67s                             | 7.71s                               | 12.87s                         | 50.11s                          | 5                              |
| cpp11sort   | 9.58s                            | 2.47s                               | **2.66s**                       | 5.47s                             | 3.42s                               | 9.9s                           | 33.5s                           | 3                              |
| AQsort      | 24.72s                           | 3.66s                               | 23.14s                               | 21.83s                            | 22.6s                               | 25.31s                         | 121.26s                         | 8                             |
| Boost       | 8.2s                             | 3.0s                                | 4.26s                                | 13.96s                            | 6.97s                               | 7.92s                          | 44.31s                          | 4                              |
| IPS$^4$o    | **4.8s**                    | 0.19s                               | 5.97s                                | 5.21s                             | 5.59s                               | **4.91s**                 | 26.67s                          | 2                              |

## Summary
Extended benchmarks (detailed in forthcoming paper) shows that IPS4o (https://github.com/ips4o) often surpasses PPQSort in raw speed. However, IPS4o relies on the external library oneTBB (https://github.com/oneapi-src/oneTBB) introducing integration complexities. PPQSort steps up as a compelling alternative due to its:

* **Competitive Speed:** Delivers performance comparable to IPS4o on most machines.
* **Hardware Agnostic:** Maintains strong performance across various hardware, potentially surpassing IPS4o on specific systems, especially ARM platforms.
* **Dependency-Free:** No external libraries are required, simplifying integration.
For applications demanding a fast, dependency-free parallel sorting solution, PPQSort is an excellent choice.

PPQSort is also used in [tracy](https://github.com/wolfpld/tracy) project.

# Running Tests and Benchmarks
Bash script for running or building specific components:
```bash
$ scripts/build.sh all
...
$ scripts/run.sh standalone
...
```
Note that the benchmark's CMake file will by default download sparse matrices (around 26GB).

# Implementation
Details about the implementation and benchmark results can be found in the following resources:
- [IT SPY Poster](https://www.itspy.cz/wp-content/uploads/2024/09/it_spy_2024_informacni_letak_37.pdf)
  - This is a brief summary poster.
- [Full master thesis text](https://dspace.cvut.cz/bitstream/handle/10467/114740/F8-DP-2024-Hevr-Gabriel-thesis.pdf?sequence=-1&isAllowed=y)
  - a very detailed text, it also includes additional information not directly related to PPQSort, but related to parallel sorting.
- [Conference paper](https://link.springer.com/chapter/10.1007/978-3-031-85697-6_10)
  - Presented on [PPAM](https://ppam.edu.pl/) (Parallel Processing & Applied Mathematics)
- Another forthcoming publication in a Concurrency and Computation: Practice and Experience journal.
