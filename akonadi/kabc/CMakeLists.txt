include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}")

########### next target ###############

set(kabcakonadi_LIB_SRC
  contactgroupbrowser.cpp
  contactgroupeditor.cpp
  contactlineedit.cpp
  itembrowser.cpp
  kabcitembrowser.cpp
  kabcitemeditor.cpp
  kabcmodel.cpp
  waitingoverlay.cpp
)

qt4_wrap_ui(kabcakonadi_LIB_SRC kabcitemeditor.ui)
qt4_wrap_ui(kabcakonadi_LIB_SRC contactgroupeditor.ui)
kde4_add_library(akonadi-kabccommon SHARED ${kabcakonadi_LIB_SRC})

target_link_libraries(akonadi-kabccommon ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_KABC_LIBS} ${KDE4_KDEUI_LIBS})
target_link_libraries(akonadi-kabccommon LINK_INTERFACE_LIBRARIES ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_KABC_LIBS} ${KDE4_KDEUI_LIBS})

set_target_properties(akonadi-kabccommon PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS akonadi-kabccommon ${INSTALL_TARGETS_DEFAULT_ARGS})
