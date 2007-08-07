find_package( Boost REQUIRED )
macro_log_feature(Boost_FOUND "boost" "Boost C++ Libraries" "http://www.boost.org/" TRUE "" "Required by Akonadi")

configure_file(akonadi-prefix.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/akonadi-prefix.h)
check_include_files(execinfo.h HAVE_EXECINFO_H)
configure_file(config-akonadi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-akonadi.h)

find_library(STRIGIQTDBUSCLIENT_LIBRARY NAMES strigiqtdbusclient
  PATHS
  /usr/lib
  /usr/local/lib
  ${LIB_INSTALL_DIR}
  $ENV{STRIGI_HOME}/lib
)

macro_log_feature(STRIGIQTDBUSCLIENT_LIBRARY "strigi indexing" "Strigi built with indexing" "http://strigi.sourceforge.net/" FALSE "" "For Akonadi tools such as the mailthreaderagent and the message searchprovider")

# taken from FindQt4.cmake to add additional includes, should probably be merged back
MACRO(QT4_ADD_DBUS_INTERFACE2 _sources _interface _basename _include)
  GET_FILENAME_COMPONENT(_infile ${_interface} ABSOLUTE)
  SET(_header ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.h)
  SET(_impl   ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.cpp)
  SET(_moc    ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.moc)
  SET(_params -m -i ${_include} -p)

  ADD_CUSTOM_COMMAND(OUTPUT ${_impl} ${_header}
      COMMAND ${QT_DBUSXML2CPP_EXECUTABLE} ${_params} ${_basename} ${_infile}
      DEPENDS ${_infile})

  SET_SOURCE_FILES_PROPERTIES(${_impl} PROPERTIES SKIP_AUTOMOC TRUE)

  QT4_GENERATE_MOC(${_header} ${_moc})

  SET(${_sources} ${${_sources}} ${_impl} ${_header} ${_moc})
  MACRO_ADD_FILE_DEPENDENCIES(${_impl} ${_moc})
ENDMACRO(QT4_ADD_DBUS_INTERFACE2)

if (XSLTPROC_EXECUTABLE)
add_subdirectory(server)
endif (XSLTPROC_EXECUTABLE)

add_subdirectory(libakonadi)
add_subdirectory(clients)
add_subdirectory(resources)
add_subdirectory(agents)
add_subdirectory(searchproviders)
add_subdirectory(kioslave)
add_subdirectory(plugins)
add_subdirectory(kabc)
add_subdirectory(kmime)
