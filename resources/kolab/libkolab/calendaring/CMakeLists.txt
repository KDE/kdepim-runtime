set (CALENDARING_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/calendaring.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/event.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/datetimeutils.cpp
    PARENT_SCOPE)

if(PYTHON_BINDINGS)
    message("building python bindings")
    add_subdirectory(python)
endif(PYTHON_BINDINGS)

if(PHP_BINDINGS)
    message("building php bindings")
    add_subdirectory(php)
endif(PHP_BINDINGS)
