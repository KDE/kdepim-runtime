project(openxchange)

# search packages used by KDE
find_package (KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)
include(CheckIncludeFiles)
find_package(KdepimLibs REQUIRED)

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

set(KDE4_ICON_DIR ${KDE4_INSTALL_DIR}/share/icons)

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
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( openxchangeresource_SRCS
  oxa/connectiontestjob.cpp
  oxa/contactutils.cpp
  oxa/davmanager.cpp
  oxa/davutils.cpp
  oxa/folder.cpp
  oxa/foldercreatejob.cpp
  oxa/folderdeletejob.cpp
  oxa/foldermodifyjob.cpp
  oxa/foldermovejob.cpp
  oxa/folderrequestjob.cpp
  oxa/foldersrequestjob.cpp
  oxa/folderutils.cpp
  oxa/incidenceutils.cpp
  oxa/object.cpp
  oxa/objectcreatejob.cpp
  oxa/objectdeletejob.cpp
  oxa/objectmodifyjob.cpp
  oxa/objectmovejob.cpp
  oxa/objectrequestjob.cpp
  oxa/objectsrequestjob.cpp
  oxa/objectutils.cpp
  oxa/oxutils.cpp
  oxa/updateusersjob.cpp
  oxa/user.cpp
  oxa/users.cpp
  oxa/useridrequestjob.cpp
  oxa/usersrequestjob.cpp

  configdialog.cpp
  openxchangeresource.cpp
)

kde4_add_ui_files( openxchangeresource_SRCS configdialog.ui )

install( FILES openxchangeresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_kcfg_files(openxchangeresource_SRCS settings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/openxchangeresource.kcfg org.kde.Akonadi.OpenXchange.Settings)
qt4_add_dbus_adaptor(openxchangeresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.OpenXchange.Settings.xml settings.h Settings
)

kde4_add_executable(akonadi_openxchange_resource RUN_UNINSTALLED ${openxchangeresource_SRCS})

target_link_libraries(akonadi_openxchange_resource ${KDE4_AKONADI_LIBS} ${KDEPIMLIBS_KABC_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KCAL_LIBS})

install(TARGETS akonadi_openxchange_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(icons)
