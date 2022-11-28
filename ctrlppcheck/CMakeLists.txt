project(Ctrlppcheck)
cmake_minimum_required(VERSION 2.8.11)

include(GNUInstallDirs)

include(cmake/versions.cmake REQUIRED)
include(cmake/options.cmake REQUIRED)
include(cmake/findDependencies.cmake REQUIRED)
include(cmake/compileroptions.cmake REQUIRED)
include(cmake/compilerDefinitions.cmake REQUIRED)

file(GLOB cfgs "cfg/*.cfg")

add_subdirectory(externals/tinyxml2)
add_subdirectory(externals/simplecpp)
add_subdirectory(lib)   # CppCheck Library 
add_subdirectory(cli)   # Client application

include(cmake/printInfo.cmake REQUIRED)
