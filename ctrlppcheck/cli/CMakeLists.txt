include_directories(${PROJECT_SOURCE_DIR}/lib/)
include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/externals/tinyxml2/)
include_directories(${PROJECT_SOURCE_DIR}/externals/simplecpp/)

file(GLOB hdrs "*.h")
file(GLOB srcs "*.cpp")
file(GLOB mainfile "main.cpp")
list(REMOVE_ITEM srcs ${mainfile})

add_library(cli_objs OBJECT ${hdrs} ${srcs}) 
add_executable(ctrlppcheck ${hdrs} ${mainfile} $<TARGET_OBJECTS:cli_objs> $<TARGET_OBJECTS:lib_objs> $<TARGET_OBJECTS:tinyxml2_objs> $<TARGET_OBJECTS:simplecpp_objs>) #$<TARGET_OBJECTS:tinyxml_objs> 

if (WIN32 AND NOT BORLAND)
    target_link_libraries(ctrlppcheck Shlwapi.lib)
endif()

install(TARGETS ctrlppcheck
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
    COMPONENT applications)

install(FILES ${cfgs}
   DESTINATION ${CFGDIR}/
   COMPONENT headers)
