
set (ICALENDAR_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/icalendar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imip.cpp
    PARENT_SCOPE)

if(PYTHON_BINDINGS)
    message("building python bindings")
    add_subdirectory(python)
endif(PYTHON_BINDINGS)

if(PHP_BINDINGS)
    message("building php bindings")
    add_subdirectory(php)
endif(PHP_BINDINGS)
