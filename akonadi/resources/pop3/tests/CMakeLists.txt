find_package(Akonadi)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIR}
    ${AKONADI_INCLUDE_DIR}
    ${KDEPIMLIBS_INCLUDE_DIRS}
)

# Stolen from kdepimlibs/akonadi/tests
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

macro(add_akonadi_isolated_test _source)
  get_filename_component(_targetName ${_source} NAME_WE)
  set(_srcList ${_source} fakeserver/fakeserver.cpp)

  kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/../pop3resource.kcfg org.kde.Akonadi.POP3.Settings)
  set(pop3settingsinterface_xml ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.POP3.Settings.xml)
  kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/../../maildir/maildirresource.kcfg org.kde.Akonadi.Maildir.Settings)
  set(maildirsettingsinterface_xml ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Maildir.Settings.xml)
  set_source_files_properties(${pop3settingsinterface_xml} PROPERTIES INCLUDE "../metatype.h")
  qt4_add_dbus_interface(_srcList ${pop3settingsinterface_xml} pop3settings)
  qt4_add_dbus_interface(_srcList ${maildirsettingsinterface_xml} maildirsettings)
  # add the dbus interace to every test (easier than adding to particular tests only)
  #qt4_add_dbus_interface(_srcList ../org.kde.krss.xml krssinterface)

  kde4_add_executable(${_targetName} TEST ${_srcList})
  target_link_libraries(${_targetName}
    ${QT_QTTEST_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${KDE4_AKONADI_LIBS}
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KMIME_LIBS}
    ${QT_QTCORE_LIBRARY}
    ${QT_QTNETWORK_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
  )

  # based on kde4_add_unit_test
  if (WIN32)
    get_target_property( _loc ${_targetName} LOCATION )
    set(_executable ${_loc}.bat)
  else (WIN32)
    set(_executable ${EXECUTABLE_OUTPUT_PATH}/${_targetName})
  endif (WIN32)
  if (UNIX)
    set(_executable ${_executable}.shell)
  endif (UNIX)

  find_program(_testrunner akonaditest)

  add_test( ${_targetName} ${_testrunner} -c ${CMAKE_CURRENT_SOURCE_DIR}/unittestenv/config.xml ${_executable} )
endmacro(add_akonadi_isolated_test)

add_akonadi_isolated_test(pop3test.cpp)
