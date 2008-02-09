# generates a D-Bus interface description from a KConfigXT file
macro( kcfg_generate_dbus_interface _kcfg _name )
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
    COMMAND ${XSLTPROC_EXECUTABLE} --stringparam interfaceName ${_name}
            ${CMAKE_SOURCE_DIR}/akonadi/libakonadi/kcfg2dbus.xsl
            ${_kcfg}
            > ${CMAKE_CURRENT_BINARY_DIR}/${_name}.xml
    DEPENDS ${CMAKE_SOURCE_DIR}/akonadi/libakonadi/kcfg2dbus.xsl
            ${_kcfg}
  )
endmacro( kcfg_generate_dbus_interface )

add_subdirectory( ical )
add_subdirectory( knut )
add_subdirectory( vcard )
if ( XSLTPROC_EXECUTABLE )
  add_subdirectory( maildir )
  add_subdirectory( nntp )
endif ( XSLTPROC_EXECUTABLE )
add_subdirectory( localbookmarks )

#This is experimental and probably experiencing bit-rot.
#So, comment out for now -- talk to Brad Hards <bradh@frogmouth.net> for more info.
#if(UNIX)
#   macro_optional_find_package(OpenChange)
#   macro_log_feature(LIBMAPI_FOUND "libmapi" "OpenChange MAPI Client Library" "http://www.openchange.org" FALSE "" "Needed to support OpenChange / Exchange servers")

#   if(LIBMAPI_FOUND)
#      add_subdirectory(openchange)
#   endif(LIBMAPI_FOUND)

#endif(UNIX)
