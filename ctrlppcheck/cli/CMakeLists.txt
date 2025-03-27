include_directories(${PROJECT_SOURCE_DIR}/lib/)
include_directories(${PROJECT_SOURCE_DIR}/externals/simplecpp/)

file(GLOB hdrs "*.h")
file(GLOB srcs "*.cpp")
file(GLOB mainfile "main.cpp")
list(REMOVE_ITEM srcs ${mainfile})

add_library(cli_objs OBJECT ${hdrs} ${srcs})

target_link_libraries(cli_objs PRIVATE tinyxml2)
add_executable(ctrlppcheck ${hdrs} ${mainfile})
target_link_libraries(ctrlppcheck PRIVATE cli_objs lib_objs simplecpp_objs)

if (WIN32 AND NOT BORLAND)
    target_link_libraries(ctrlppcheck Shlwapi.lib)
endif()

install(TARGETS ctrlppcheck
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    COMPONENT applications)

install(FILES ${cfgs}
   DESTINATION ${CFGDIR}/
   COMPONENT headers)
