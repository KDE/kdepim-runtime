set(mbox_LIB_SRCS mbox.cpp)

add_subdirectory(tests)

kde4_add_library(mbox SHARED ${mbox_LIB_SRCS})

target_link_libraries(mbox
  ${KDE4_KDECORE_LIBS}
  ${KDEPIMLIBS_KPIMUTILS_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
)

set_target_properties(mbox PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS mbox ${INSTALL_TARGETS_DEFAULT_ARGS})
