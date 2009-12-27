project(filestoreresources)

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
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

set( storeresourceshared_SRCS
  resourcebase.cpp
)

########### next target ###############

set( augmentedmboxresource_SRCS
  augmentedmboxresource.cpp
  ${storeresourceshared_SRCS}
)

install( FILES augmentedmboxresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_kcfg_files(augmentedmboxresource_SRCS augmentedmboxresourcesettings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/augmentedmboxresource.kcfg org.kde.Akonadi.AugmentedMBoxResource.Settings)
qt4_add_dbus_adaptor(augmentedmboxresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.AugmentedMBoxResource.Settings.xml augmentedmboxresourcesettings.h Settings augmentedmboxsettingsadaptor
)

kde4_add_executable(akonadi_augmentedmbox_resource RUN_UNINSTALLED ${augmentedmboxresource_SRCS})

target_link_libraries(akonadi_augmentedmbox_resource akonadi-filestore ${KDE4_AKONADI_KMIME_LIBS} ${KDE4_AKONADI_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDECORE_LIBS})

install(TARGETS akonadi_augmentedmbox_resource ${INSTALL_TARGETS_DEFAULT_ARGS})

########### next target ###############

set( mappedvcardresource_SRCS
  mappedvcardresource.cpp
  ${storeresourceshared_SRCS}
)

install( FILES mappedvcardresource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

kde4_add_kcfg_files(mappedvcardresource_SRCS mappedvcardresourcesettings.kcfgc)
kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/mappedvcardresource.kcfg org.kde.Akonadi.MappedVCardResource.Settings)
qt4_add_dbus_adaptor(mappedvcardresource_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.MappedVCardResource.Settings.xml mappedvcardresourcesettings.h Settings mappedvcardsettingsadaptor
)

kde4_add_executable(akonadi_mappedvcard_resource RUN_UNINSTALLED ${mappedvcardresource_SRCS})

target_link_libraries(akonadi_mappedvcard_resource akonadi-filestore ${KDE4_AKONADI_KABC_LIBS} ${KDE4_AKONADI_LIBS} ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDECORE_LIBS})

install(TARGETS akonadi_mappedvcard_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
