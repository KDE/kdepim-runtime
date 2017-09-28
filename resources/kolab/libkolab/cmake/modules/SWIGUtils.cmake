

macro (generatePHPBindings MODULE_NAME INTERFACE_FILE)

    set(KOLAB_SWIG_PHP_SOURCE_FILE php_${MODULE_NAME}_wrapper.cpp)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${KOLAB_SWIG_PHP_SOURCE_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}.php
        COMMAND ${SWIG} -v -c++ -php -I${Libkolabxml_INCLUDES} -module ${MODULE_NAME} -o ${CMAKE_CURRENT_BINARY_DIR}/${KOLAB_SWIG_PHP_SOURCE_FILE} ${INTERFACE_FILE}
        COMMENT "Generating php bindings"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${INTERFACE_FILE} kolab
        VERBATIM
    )

    SET_SOURCE_FILES_PROPERTIES(${KOLAB_SWIG_PHP_SOURCE_FILE} PROPERTIES GENERATED 1)
    ADD_CUSTOM_TARGET(generate_${MODULE_NAME}_php_bindings ALL DEPENDS ${KOLAB_SWIG_PHP_SOURCE_FILE})

    #Compile PHP Bindings
    # Since there is no php library we can't compile with -Wl,--no-undefined
    set(CMAKE_SHARED_LINKER_FLAGS "")

    if (APPLE)
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flat_namespace -undefined suppress" )
    else()
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused" )
    endif()
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers -Wno-undef" )

    # Try to find PHP5
    find_path(PHP_INCLUDE_DIR NAMES main/php.h PATH_SUFFIXES php php5)
    find_program(PHP_EXECUTABLE NAMES php)

    # Libkolab needs PHP >= 5.3
    set(PHP_MIN_VERSION 50300)

    # Find where to install the extension files if it's not defined
    if(NOT DEFINED PHP_INSTALL_DIR)
        find_program(PHP_CONFIG_EXECUTABLE NAMES php-config)
        if(PHP_CONFIG_EXECUTABLE)
            execute_process(COMMAND ${PHP_CONFIG_EXECUTABLE} --extension-dir
                OUTPUT_VARIABLE _php_extensions_dir
                )
            string(REGEX REPLACE "\n" "" _php_extensions_dir "${_php_extensions_dir}")
            set(PHP_INSTALL_DIR ${_php_extensions_dir} CACHE STRING "Install directory for PHP bindings.")
        else()
            set(PHP_INSTALL_DIR ${LIB_INSTALL_DIR}/extensions)
        endif()
    endif()

    if(PHP_INCLUDE_DIR AND PHP_EXECUTABLE)
        file(READ ${PHP_INCLUDE_DIR}/main/php_version.h PHP_VERSION_CONTENT)
        string(REGEX MATCH "#define PHP_VERSION_ID[ ]*[0-9]*\n" _PHP_VERSION_ID_MATCH ${PHP_VERSION_CONTENT})
        if(_PHP_VERSION_ID_MATCH)
            string(REGEX REPLACE "#define PHP_VERSION_ID[ ]*([0-9]*)\n" "\\1" PHP_VERSION_ID ${_PHP_VERSION_ID_MATCH})
        endif()

        # Include the needed PHP5 subdirs
        set(PHP_INCLUDE_DIRS ${PHP_INCLUDE_DIR} ${PHP_INCLUDE_DIR}/main ${PHP_INCLUDE_DIR}/TSRM ${PHP_INCLUDE_DIR}/Zend )
    endif()

    if(NOT PHP_VERSION_ID VERSION_LESS ${PHP_MIN_VERSION})
        include_directories(${PHP_INCLUDE_DIRS})
        add_library(${MODULE_NAME}_phpbindings SHARED ${KOLAB_SWIG_PHP_SOURCE_FILE})
        target_link_libraries(${MODULE_NAME}_phpbindings kolab)
        set_target_properties(${MODULE_NAME}_phpbindings PROPERTIES OUTPUT_NAME ${MODULE_NAME})
        set_target_properties(${MODULE_NAME}_phpbindings PROPERTIES PREFIX "")

        #     configure_file(test.php ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

        install(TARGETS ${MODULE_NAME}_phpbindings LIBRARY DESTINATION ${PHP_INSTALL_DIR})
        install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}.php
            DESTINATION ${PHP_INSTALL_DIR}
            )

    else()
        message(WARNING "not building php bindings because php was not found")
    endif()

endmacro()


macro(generatePythonBindings MODULE_NAME INTERFACE_FILE)

    find_package(SWIG REQUIRED)

    # Compile Python Bindings

    find_package(PythonLibs)

    if (NOT PYTHONLIBS_FOUND)
        message("python libs not found, not building python bindings")
        return()
    endif()
    message("found python include dirs: ${PYTHON_INCLUDE_DIRS} ${PYTHON_INCLUDE_PATH}")

    set(KOLAB_SWIG_PYTHON_SOURCE_FILE ${CMAKE_CURRENT_BINARY_DIR}/python_${MODULE_NAME}_wrapper.cpp)
    set(KOLAB_SWIG_PYTHON_HEADER_FILE ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}.py)

    add_custom_command(OUTPUT ${KOLAB_SWIG_PYTHON_SOURCE_FILE} ${KOLAB_SWIG_PYTHON_HEADER_FILE}
        COMMAND ${SWIG} -v -c++ -python -module ${MODULE_NAME} -I${Libkolabxml_INCLUDES} -o ${KOLAB_SWIG_PYTHON_SOURCE_FILE} ${INTERFACE_FILE}
        COMMENT "Generating python bindings"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${INTERFACE_FILE} kolab
        VERBATIM
        )

    SET_SOURCE_FILES_PROPERTIES(${KOLAB_SWIG_PYTHON_SOURCE_FILE} PROPERTIES GENERATED 1)

    #${PYTHON_INCLUDE_PATH} is for backwards compatibility (el6)
    include_directories(${PYTHON_INCLUDE_DIRS} ${PYTHON_INCLUDE_PATH})

    set(PYTHON_MODULE_PREFIX "_")
    python_add_module(${MODULE_NAME} ${KOLAB_SWIG_PYTHON_SOURCE_FILE})

    #cmake 2.6.4 fails to respect the module prefix
    SET_TARGET_PROPERTIES(${MODULE_NAME} PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}")

    target_link_libraries(${MODULE_NAME} kolab ${PYTHON_LIBRARIES})
    # configure_file(test.py ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

    set(PYTHON_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/pythonbindings" CACHE STRING "Install directory for python bindings.")

    install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/kolab)

    install( FILES
        ${KOLAB_SWIG_PYTHON_HEADER_FILE}
        DESTINATION ${PYTHON_INSTALL_DIR}/kolab
    )

endmacro()

