include_directories(
    ${CMAKE_SOURCE_DIR}/akonadi
    ${CMAKE_CURRENT_SOURCE_DIR}/libmbox
    ${QT_QTDBUS_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

# mbox access library
add_subdirectory(libmbox)


########### next target ###############

set( mboxresource_SRCS
  deleteditemsattribute.cpp
  mboxresource.cpp
  configdialog.cpp
)

install( FILES mboxresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_ui_files(mboxresource_SRCS settings.ui)
kde4_add_kcfg_files(mboxresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/mboxresource.kcfg org.kde.Akonadi.Mbox.Settings)
qt4_add_dbus_adaptor(mboxresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Mbox.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_mbox_resource RUN_UNINSTALLED ${mboxresource_SRCS})

target_link_libraries(akonadi_mbox_resource mbox ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${QT_QTDBUS_LIBRARY} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KMIME_LIBS})

install(TARGETS akonadi_mbox_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

