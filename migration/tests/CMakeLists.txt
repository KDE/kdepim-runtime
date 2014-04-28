project(migrationtests)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include(AkonadiMacros)
set(KDEPIMLIBS_RUN_ISOLATED_TESTS TRUE)
set(KDEPIMLIBS_RUN_SQLITE_ISOLATED_TESTS TRUE)

set(_srcList
  testnotesmigration.cpp
  ../kjots/kjotsmigrator.cpp
  ../kres/kresmigratorbase.cpp
  ${MIGRATION_AKONADI_SHARED_SOURCES}
)
include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../kjots
  ${CMAKE_CURRENT_BINARY_DIR}/../kres
)

macro(add_resource_iface _kcfgFile _ifaceName _className)
  kcfg_generate_dbus_interface(${kdepim-runtime_SOURCE_DIR}/resources/${_kcfgFile} ${_ifaceName})
  string(TOLOWER ${_className} _codeFile)
  qt4_add_dbus_interface(_srcList
    ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml ${_codeFile} ${_className}
  )
endmacro()

add_resource_iface( maildir/maildirresource.kcfg
                    org.kde.Akonadi.Maildir.Settings MaildirSettings )

add_akonadi_isolated_test_advanced( testgidmigration.cpp "${MIGRATION_AKONADI_SHARED_SOURCES};../gid/gidmigrationjob.cpp" "" )

kde4_add_executable(testmigratorbase testmigratorbase.cpp ../migratorbase.cpp)
target_link_libraries(testmigratorbase
    gidmigration
    ${KDE4_KDECORE_LIBS}
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${QT_QTTEST_LIBRARY}
)
add_test(testmigratorbase testmigratorbase)
