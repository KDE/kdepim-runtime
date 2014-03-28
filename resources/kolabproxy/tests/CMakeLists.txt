
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )
set( RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )

foreach( _file ${kolabproxyresource_shared_SRCS} )
  list( APPEND kolabproxy_shared_relative_SRCS "../${_file}" )
endforeach()

kde4_add_library(kolabresourcetest STATIC
    ${kolabproxy_shared_relative_SRCS}
    ${AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES}
    testutils.cpp
)

kde4_add_unit_test(kolabconvertertest kolabconvertertest.cpp)
target_link_libraries(kolabconvertertest
    kolabresourcetest
    ${Libkolab_LIBRARIES}
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${QT_QTXML_LIBRARY}
    ${QT_QTDBUS_LIBRARY}
    ${KDE4_KIO_LIBS}
    ${KDEPIMLIBS_KABC_LIBS}
    ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
    ${KDEPIMLIBS_KMIME_LIBS}
    ${QT_QTTEST_LIBRARY}
    kdepim-copy
    ${KDEPIMLIBS_KCALCORE_LIBS}
)

#akonadi_add_resourcetest( imap-kolab imaptest-kolab.es )
#akonadi_add_resourcetest( imap-dovecot imaptest-dovecot.es )
#akonadi_add_resourcetest( kolab kolabtest.es )

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
include(AkonadiMacros)
set(KDEPIMLIBS_RUN_ISOLATED_TESTS TRUE)
set(KDEPIMLIBS_RUN_SQLITE_ISOLATED_TESTS TRUE)

add_akonadi_isolated_test_advanced(proxyintegrationtest.cpp "" "kolabresourcetest;${Libkolab_LIBRARIES}")
add_akonadi_isolated_test_advanced(imapsignaltest.cpp "" "kolabresourcetest;${Libkolab_LIBRARIES}")
add_akonadi_isolated_test_advanced(clientsidetest.cpp "" "kolabresourcetest;${Libkolab_LIBRARIES}")
