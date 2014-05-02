include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(tests)

########### next target ###############

set(kmindexreader_LIB_SRCS
 kmindexreader.cpp
)

add_library(kmindexreader ${LIBRARY_TYPE} ${kmindexreader_LIB_SRCS})

target_link_libraries(kmindexreader
    KF5::AkonadiMime
)

set_target_properties(kmindexreader PROPERTIES VERSION ${KDEPIM_LIB_VERSION} SOVERSION ${KDEPIM_LIB_SOVERSION} )

install(TARGETS kmindexreader ${INSTALL_TARGETS_DEFAULT_ARGS})
