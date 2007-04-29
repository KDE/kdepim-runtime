
add_subdirectory( tests )


set(maildir_LIB_SRCS maildir.cpp)

kde4_automoc(${maildir_LIB_SRCS})

kde4_add_library(maildir SHARED ${maildir_LIB_SRCS})

target_link_libraries(maildir ${KDE4_KDECORE_LIBS} )

set_target_properties(maildir PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS maildir  DESTINATION ${LIB_INSTALL_DIR})

