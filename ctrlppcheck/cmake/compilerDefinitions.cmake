if (UNIX)
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        add_definitions(-D_GLIBCXX_DEBUG)
    endif()
    if (HAVE_RULES)
        add_definitions(-DHAVE_RULES -DTIXML_USE_STL)
    endif()
    add_definitions(-DCFGDIR="${CFGDIR}")
endif()
