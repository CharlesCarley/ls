cmake_minimum_required(VERSION 3.0)
project(ListDir)
list(APPEND 
    CMAKE_MODULE_PATH 
    ${ListDir_SOURCE_DIR}/CMake)
include(StaticRuntime)
subdirs(Source)