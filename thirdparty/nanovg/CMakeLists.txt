cmake_minimum_required(VERSION 3.16)
project(nanovg)

add_library(nanovg SHARED
    src/nanovg.c
    src/libnanovg.c
    src/glad.c
)
