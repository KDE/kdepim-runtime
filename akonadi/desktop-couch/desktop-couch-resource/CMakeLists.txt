cmake_minimum_required(VERSION 2.6)
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(CheckIncludeFiles)
include(MacroBoolTo01)
find_package(KdepimLibs 4.2.87 REQUIRED)
find_package( Boost REQUIRED )

#temporary
find_library( COUCHDBQT_LIBRARY
    NAMES
    couchdb-qt
    PATHS
    ${LIB_INSTALL_DIR}
    ${KDE4_LIB_DIR}
)

find_path (COUCHDBQT_INCLUDE_DIR
    NAMES
    couchdb-qt.h
    PATH_SUFFIXES
    couchdbqt
    PATHS
    ${INCLUDE_INSTALL_DIR}
    ${KDE4_INCLUDE_DIR}
  )

message( WARNING ${COUCHDBQT_LIBRARY} )
message( WARNING ${COUCHDBQT_INCLUDE_DIR} )

include_directories(
    ${kdepim-runtime_SOURCE_DIR}
    ${QT_QTDBUS_INCLUDE_DIR}
    ${kdepim-runtime_BINARY_DIR}
    ${AKONADI_INCLUDE_DIR}
    ${Boost_INCLUDE_DIR}
    ${KDE4_INCLUDES}
    ${KDEPIMLIBS_INCLUDE_DIRS}
    ${COUCHDBQT_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( desktop-couch-resource_SRCS
  desktop-couch-resource.cpp
)

install( FILES desktop-couch-resource.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

#kde4_add_kcfg_files(desktop-couch-resource_SRCS settings.kcfgc)
#kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/desktop-couch-resource.kcfg org.kde.Akonadi.desktop-couch.Settings)
#qt4_add_dbus_adaptor(desktop-couch-resource_SRCS
#  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.desktop-couch.Settings.xml settings.h Settings
#)

kde4_add_executable(akonadi_desktop-couch_resource RUN_UNINSTALLED ${desktop-couch-resource_SRCS})

target_link_libraries( akonadi_desktop-couch_resource
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KABC_LIBRARY}
    ${COUCHDBQT_LIBRARY}
)

install(TARGETS akonadi_desktop-couch_resource ${INSTALL_TARGETS_DEFAULT_ARGS})
