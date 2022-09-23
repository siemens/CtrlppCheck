project(Ctrlppcheck)
cmake_minimum_required(VERSION 2.8.11)

include(GNUInstallDirs)

include(cmake/versions.cmake REQUIRED)
include(cmake/options.cmake REQUIRED)
include(cmake/findDependencies.cmake REQUIRED)
include(cmake/compileroptions.cmake REQUIRED)
include(cmake/compilerDefinitions.cmake REQUIRED)
# include(cmake/buildFiles.cmake REQUIRED)

#if (WIN32)
#endif (WIN32)

#include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
# conan_basic_setup()


file(GLOB cfgs "cfg/*.cfg")

#if (BUILD_TESTS)
#    enable_testing()
#endif()

add_subdirectory(externals/tinyxml2)
add_subdirectory(externals/simplecpp)
add_subdirectory(lib)   # CppCheck Library 
add_subdirectory(cli)   # Client application
#add_subdirectory(test)  # Tests

include(cmake/printInfo.cmake REQUIRED)
