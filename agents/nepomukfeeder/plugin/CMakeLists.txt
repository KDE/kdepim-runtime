
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

kde4_add_library(nepomukfeederpluginlib nepomukfeederplugin.cpp nepomukfeederutils.cpp)
target_link_libraries(nepomukfeederpluginlib
    ${KDE4_KDECORE_LIBS}
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${QT_QTCORE_LIBRARY}
    nepomukdatamanagement-copy
    niefast
)

########### install files ###############

install(TARGETS nepomukfeederpluginlib
  DESTINATION ${LIB_INSTALL_DIR})

