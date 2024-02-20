cmake_minimum_required(VERSION 3.17)

project(PFFFT VERSION 4.5.2)

set(CMAKE_C_STANDARD 11)

add_library(pffft STATIC
    pffft.c pffft.h
    fftpack.c fftpack.h)
