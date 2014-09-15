
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )
set( RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )

foreach( _file ${kolabproxyresource_shared_SRCS} )
  list( APPEND kolabproxy_shared_relative_SRCS "../${_file}" )
endforeach()

add_library(kolabresourcetest STATIC
    ${kolabproxy_shared_relative_SRCS}
    testutils.cpp
)

add_executable(kolabconvertertest kolabconvertertest.cpp)
add_test(kolabconvertertest kolabconvertertest)
ecm_mark_as_test(kolabconvertertest)
target_link_libraries(kolabconvertertest
    kolabresourcetest
    ${Libkolab_LIBRARIES}
    KF5::AkonadiCore
    KF5::KIOCore
    KF5::Abc
    KF5::AkonadiMime
    KF5::Mime
    Qt5::Test
    KF5::Kolab
    KF5::CalendarCore
    akonadi-singlefileresource
)

#akonadi_add_resourcetest( imap-kolab imaptest-kolab.es )
#akonadi_add_resourcetest( imap-dovecot imaptest-dovecot.es )
#akonadi_add_resourcetest( kolab kolabtest.es )

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
set(KDEPIMLIBS_RUN_ISOLATED_TESTS TRUE)
set(KDEPIMLIBS_RUN_SQLITE_ISOLATED_TESTS TRUE)

add_akonadi_isolated_test_advanced(proxyintegrationtest.cpp "" "kolabresourcetest;${Libkolab_LIBRARIES}")
add_akonadi_isolated_test_advanced(imapsignaltest.cpp "" "kolabresourcetest;${Libkolab_LIBRARIES}")
add_akonadi_isolated_test_advanced(clientsidetest.cpp "" "kolabresourcetest;${Libkolab_LIBRARIES}")
