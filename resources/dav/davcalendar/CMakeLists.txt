project(davcalendar)

# search packages used by KDE
find_package (KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)
include(CheckIncludeFiles)
find_package (KdepimLibs REQUIRED)

find_program(XSLTPROC_EXECUTABLE xsltproc)
macro_log_feature(XSLTPROC_EXECUTABLE "xsltproc" "The command line XSLT processor from libxslt" "http://xmlsoft.org/XSLT/" FALSE "" "Needed for building Akonadi resources. Recommended.")

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

if(WIN32)
    set(LIB_INSTALL_DIR ${LIB_INSTALL_DIR}
                        RUNTIME DESTINATION ${BIN_INSTALL_DIR}
                        LIBRARY DESTINATION ${LIB_INSTALL_DIR}
                        ARCHIVE DESTINATION ${LIB_INSTALL_DIR} )
endif(WIN32)

set(KDE4_ICON_DIR ${KDE4_INSTALL_DIR}/share/icons)

include_directories(
    ${KDE4_INCLUDES}
    ${KDEPIMLIBS_INCLUDE_DIRS}
    ../common/
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( davcalendarresource_SRCS
  ../common/davcollection.cpp
  ../common/davcollectionsfetchjob.cpp
  ../common/davcollectionmodifyjob.cpp
  ../common/davcollectionsmultifetchjob.cpp
  ../common/davprotocolbase.cpp
  ../common/davitem.cpp
  ../common/davitemcreatejob.cpp
  ../common/davitemdeletejob.cpp
  ../common/davitemfetchjob.cpp
  ../common/davitemmodifyjob.cpp
  ../common/davitemslistjob.cpp
  ../common/davmanager.cpp
  ../common/davutils.cpp
  ../common/etagcache.cpp

  caldavprotocol.cpp
  carddavprotocol.cpp
  configdialog.cpp
  davcalendarresource.cpp
  davprotocolattribute.cpp
  groupdavprotocol.cpp
  settings.cpp
  urlconfigurationdialog.cpp
)

install( FILES davcalendarresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_kcfg_files(davcalendarresource_SRCS settingsbase.kcfgc)
kde4_add_ui_files(davcalendarresource_SRCS configdialog.ui urlconfigurationdialog.ui)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/davcalendarresource.kcfg org.kde.Akonadi.davCalendar.Settings)
qt4_add_dbus_adaptor(davcalendarresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.davCalendar.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_davcalendar_resource RUN_UNINSTALLED ${davcalendarresource_SRCS})

target_link_libraries(akonadi_davcalendar_resource ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${QT_QTXML_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KABC_LIBS} ${KDEPIMLIBS_KCAL_LIBS} QtXmlPatterns akonadi-kcal)

install(TARGETS akonadi_davcalendar_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
