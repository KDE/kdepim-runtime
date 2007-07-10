
add_subdirectory( tests )


set(maildir_LIB_SRCS maildir.cpp)


kde4_add_library(maildir SHARED ${maildir_LIB_SRCS})

target_link_libraries(maildir ${KDE4_KDECORE_LIBS} )

set_target_properties(maildir PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS maildir  DESTINATION ${LIB_INSTALL_DIR})

