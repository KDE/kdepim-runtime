project (maildispatcheragent)

# search packages used by KDE
find_package(KDE4 REQUIRED)
find_package(KdepimLibs REQUIRED)
find_package(Akonadi REQUIRED)

include (KDE4Defaults)
include (MacroLibrary)
include(MacroOptionalAddSubdirectory)

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

include_directories(
    ${KDE4_INCLUDES}
    ${KDEPIMLIBS_INCLUDE_DIRS}
    ${AKONADI_INCLUDE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( maildispatcheragent_SRCS
  maildispatcheragent.cpp
  dispatchmodeattribute.cpp
  configdialog.cpp
)

add_definitions( -DKDE_DEFAULT_DEBUG_AREA=5266 )

find_program(XSLTPROC_EXECUTABLE xsltproc)
macro_log_feature(XSLTPROC_EXECUTABLE "xsltproc" "The command line XSLT processor from libxslt"
                  "http://xmlsoft.org/XSLT/" FALSE "" "Needed for building Akonadi resources. Recommended.")

if (XSLTPROC_EXECUTABLE)
  # generates a D-Bus interface description from a KConfigXT file
  macro( kcfg_generate_dbus_interface _kcfg _name )
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
      COMMAND ${XSLTPROC_EXECUTABLE} --stringparam interfaceName ${_name}
              ${KDEPIMLIBS_DATA_DIR}/akonadi-kde/kcfg2dbus.xsl
              ${_kcfg}
              > ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
      DEPENDS ${KDEPIMLIBS_DATA_DIR}/akonadi-kde/kcfg2dbus.xsl
              ${_kcfg}
    )
  endmacro( kcfg_generate_dbus_interface )
endif (XSLTPROC_EXECUTABLE)

kde4_add_ui_files(maildispatcheragent_SRCS settings.ui)
kde4_add_kcfg_files(maildispatcheragent_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/maildispatcheragent.kcfg org.kde.Akonadi.MailDispatcher.Settings)
qt4_add_dbus_adaptor(maildispatcheragent_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.MailDispatcher.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_maildispatcher_agent RUN_UNINSTALLED ${maildispatcheragent_SRCS})

target_link_libraries(akonadi_maildispatcher_agent ${KDE4_AKONADI_LIBS} ${KDE4_AKONADI_KMIME_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KMIME_LIBS} ${KDE4_MAILTRANSPORT_LIBS} outboxinterface )

install(TARGETS akonadi_maildispatcher_agent ${INSTALL_TARGETS_DEFAULT_ARGS})
install( FILES maildispatcheragent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )
