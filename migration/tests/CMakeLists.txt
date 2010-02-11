project(migrationtests)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

set(_srcList
  testnotesmigration.cpp
  ../kjots/kjotsmigrator.cpp
  ${MIGRATION_AKONADI_SHARED_SOURCES}
)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../kjots
)

macro(add_resource_iface _kcfgFile _ifaceName _className)
  kcfg_generate_dbus_interface(${kdepim-runtime_SOURCE_DIR}/resources/${_kcfgFile} ${_ifaceName})
  string(TOLOWER ${_className} _codeFile)
  qt4_add_dbus_interface(_srcList
    ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml ${_codeFile} ${_className}
  )
endmacro(add_resource_iface)

add_resource_iface( maildir/maildirresource.kcfg
                    org.kde.Akonadi.Maildir.Settings MaildirSettings )


kde4_add_executable(testnotesmigration TEST ${_srcList})
target_link_libraries(testnotesmigration
  ${QT_QTTEST_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  ${KDE4_KDECORE_LIBS}
  ${KDEPIMLIBS_MAILTRANSPORT_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${QT_QTXML_LIBRARY}
  maildir
)


# based on kde4_add_unit_test
if (WIN32)
  get_target_property( _loc testnotesmigration LOCATION )
  set(_executable ${_loc}.bat)
else (WIN32)
  set(_executable ${EXECUTABLE_OUTPUT_PATH}/testnotesmigration)
endif (WIN32)
if (UNIX)
  set(_executable ${_executable}.shell)
endif (UNIX)

find_program(_testrunner akonaditest)

add_test( testnotesmigration ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config.xml ${_executable} )




add_akonadi_isolated_test( testnotesmigration.cpp )

