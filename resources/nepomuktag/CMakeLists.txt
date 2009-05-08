########### next target ###############

if ( NEPOMUK_FOUND )

set( nepomuktagresource_SRCS
  nepomuktagresource.cpp
)

install( FILES nepomuktagresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

qt4_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/nepomuktagresource.h org.kde.Akonadi.NepomukTag.Resource.xml )
qt4_add_dbus_adaptor( nepomuktagresource_SRCS
     ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.NepomukTag.Resource.xml
        nepomuktagresource.h NepomukTagResource
     )
kde4_add_executable(akonadi_nepomuktag_resource RUN_UNINSTALLED ${nepomuktagresource_SRCS})

target_link_libraries(akonadi_nepomuktag_resource ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTDBUS_LIBRARY} ${QT_QTCORE_LIBRARY} ${NEPOMUK_LIBRARIES} )

install(TARGETS akonadi_nepomuktag_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

endif ( NEPOMUK_FOUND )

