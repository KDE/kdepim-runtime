include_directories( ${Boost_INCLUDE_DIR} )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

find_program(XSLTPROC_EXECUTABLE xsltproc)
macro_log_feature(XSLTPROC_EXECUTABLE "xsltproc" "The command line XSLT processor from libxslt" "http://xmlsoft.org/XSLT/" TRUE "" "Required for building Akonadi.")
if(NOT XSLTPROC_EXECUTABLE)
  macro_display_feature_log()
endif(NOT XSLTPROC_EXECUTABLE)

# generates a D-Bus interface description from a KConfigXT file
macro(kcfg_generate_dbus_interface _kcfg _name)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
    COMMAND ${XSLTPROC_EXECUTABLE} --stringparam interfaceName ${_name}
    ${KDEPIMLIBS_DATA_DIR}/akonadi-kde/kcfg2dbus.xsl
    ${_kcfg}
    > ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
    DEPENDS ${KDEPIMLIBS_DATA_DIR}/akonadi-kde/kcfg2dbus.xsl
    ${_kcfg}
    )
endmacro(kcfg_generate_dbus_interface)

########### next target ###############

set( microblogresource_SRCS
  microblog.cpp
  configdialog.cpp
  settings.cpp
  communication.cpp
  idattribute.cpp
)

kcfg_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/microblogresource.kcfg org.kde.Akonadi.Microblog.Settings)
qt4_add_dbus_adaptor( microblogresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Microblog.Settings.xml settings.h Settings
)

kde4_add_ui_files(microblogresource_SRCS microblogconfig.ui)
kde4_add_kcfg_files(microblogresource_SRCS settingsbase.kcfgc)
kde4_add_executable(akonadi_microblog_resource RUN_UNINSTALLED ${microblogresource_SRCS})

target_link_libraries(akonadi_microblog_resource ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTDBUS_LIBRARY} ${QT_QTCORE_LIBRARY} ${QT_QTXML_LIBRARY} ${KDEPIMLIBS_KPIMUTILS_LIBS} ${KDEPIMLIBS_MICROBLOG_LIBS})

install(TARGETS akonadi_microblog_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
install( FILES microblog.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )


