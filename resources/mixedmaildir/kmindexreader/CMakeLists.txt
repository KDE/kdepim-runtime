include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(tests)

########### next target ###############

set(kmindexreader_LIB_SRCS
 kmindexreader.cpp
 ../mixedmaildir_debug.cpp
)

add_library(kmindexreader ${LIBRARY_TYPE} ${kmindexreader_LIB_SRCS})
generate_export_header(kmindexreader BASE_NAME kmindexreader)

target_link_libraries(kmindexreader
    KF5::AkonadiMime
)

set_target_properties(kmindexreader PROPERTIES VERSION ${KDEPIMRUNTIME_LIB_VERSION} SOVERSION ${KDEPIMRUNTIME_LIB_SOVERSION} )

install(TARGETS kmindexreader ${INSTALL_TARGETS_DEFAULT_ARGS})
