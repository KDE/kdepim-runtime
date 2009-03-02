project(kjots_resource)

#########################################################################
# AKONADI RESOURCE SECTION
#########################################################################

include_directories(
  ${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

# Temporary for development
FILE(WRITE ${PROJECT_BINARY_DIR}/datadir.h "#define DATADIR \"" ${PROJECT_SOURCE_DIR}../kjots_common/data/ "\"\n\n" )
include_directories( ${PROJECT_BINARY_DIR} )

########### next target ###############

set( kjotsresource_SRCS
  kjotsresource.cpp
  kjotsbook.cpp
)

install(FILES kjotsresource.desktop
        DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents" )

# kde4_add_kcfg_files(kjotsresource_SRCS settings.kcfgc)
# kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/kjotsresource.kcfg org.kde.Akonadi.kjotsresource.Settings)
# qt4_add_dbus_adaptor(kjotsresource_SRCS
#   ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.kjotsresource.Settings.xml settings.h Settings
# )

kde4_add_executable(akonadi_kjots_resource RUN_UNINSTALLED ${kjotsresource_SRCS})

target_link_libraries(akonadi_kjots_resource
                      kjots_common
#                       ${QT_QTDBUS_LIBRARY}
                      )

install(TARGETS akonadi_kjots_resource
        ${INSTALL_TARGETS_DEFAULT_ARGS})

