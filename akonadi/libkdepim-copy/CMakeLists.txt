
add_definitions(-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)

#add_definitions(-DKDE_DEFAULT_DEBUG_AREA=5300)

########### next target ###############


set(kdepim-copy_LIB_SRCS
    addresseeview.cpp
    calendardiffalgo.cpp
    diffalgo.cpp
    htmldiffalgodisplay.cpp
    kaccount.cpp
    kincidencechooser.cpp
    ksubscription.cpp
    sendsmsdialog.cpp
    uistatesaver.cpp
)

kde4_add_library(kdepim-copy SHARED ${kdepim-copy_LIB_SRCS})

target_link_libraries(kdepim-copy
  ${KDE4_KDE3SUPPORT_LIBS}
  ${KDE4_KPARTS_LIBS}
  ${KDEPIMLIBS_KCAL_LIBS}
  kimproxy
)

set_target_properties(kdepim-copy PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})
install(TARGETS kdepim-copy ${INSTALL_TARGETS_DEFAULT_ARGS})
