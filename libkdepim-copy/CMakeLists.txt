
add_subdirectory( tests )

#add_definitions(-DKDE_DEFAULT_DEBUG_AREA=5300)

########### next target ###############


set(kdepim-copy_LIB_SRCS
    calendardiffalgo.cpp
    diffalgo.cpp
    htmldiffalgodisplay.cpp
    kdatepickerpopup.cpp
    kincidencechooser.cpp
)

kde4_add_library(kdepim-copy ${LIBRARY_TYPE} ${kdepim-copy_LIB_SRCS})

target_link_libraries(kdepim-copy
  ${KDE4_KDEUI_LIBS}
  ${KDEPIMLIBS_KCALCORE_LIBS}
  ${KDEPIMLIBS_KCALUTILS_LIBS}
)

set_target_properties(kdepim-copy PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})
install(TARGETS kdepim-copy ${INSTALL_TARGETS_DEFAULT_ARGS})
