cmake_minimum_required(VERSION 3.15)
project(tinyxml2 VERSION 9.0.0)

##
## Honor tinyxml2_SHARED_LIBS to match install interface
##

if (DEFINED tinyxml2_SHARED_LIBS)
    set(BUILD_SHARED_LIBS "${tinyxml2_SHARED_LIBS}")
endif ()

##
## Main library build
##

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

add_library(tinyxml2_objs tinyxml2.cpp tinyxml2.h)

# Uncomment the following line to require C++11 (or greater) to use tinyxml2
# target_compile_features(tinyxml2_objs PUBLIC cxx_std_11)
target_include_directories(tinyxml2_objs PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

