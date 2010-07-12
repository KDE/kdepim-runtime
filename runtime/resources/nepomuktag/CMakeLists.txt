########### next target ###############

include_directories( ${SOPRANO_INCLUDE_DIR} )

set( nepomuktagresource_SRCS
  nepomuktagresource.cpp
)

install( FILES nepomuktagresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

qt4_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/nepomuktagresource.h org.kde.Akonadi.NepomukTag.Resource.xml )
qt4_add_dbus_adaptor( nepomuktagresource_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.NepomukTag.Resource.xml
        nepomuktagresource.h NepomukTagResource
     )
kde4_add_executable(akonadi_nepomuktag_resource ${nepomuktagresource_SRCS})

if (Q_WS_MAC)
  set_target_properties(akonadi_nepomuktag_resource PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/../Info.plist.template)
  set_target_properties(akonadi_nepomuktag_resource PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.Akonadi.NepomukTag")
  set_target_properties(akonadi_nepomuktag_resource PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "KDE Akonadi NepomukTag Resource")
endif (Q_WS_MAC)

target_link_libraries(akonadi_nepomuktag_resource
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEUI_LIBS}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTCORE_LIBRARY}
  ${SOPRANO_LIBRARIES}
  ${NEPOMUK_LIBRARIES}
)

install(TARGETS akonadi_nepomuktag_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

