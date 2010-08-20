
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )

foreach( _file ${kolabproxyresource_shared_SRCS} )
  list( APPEND kolabproxy_shared_relative_SRCS "../${_file}" )
endforeach()

kde4_add_unit_test(kolabconvertertest ${kolabproxy_shared_relative_SRCS} ${AKONADI_COLLECTIONATTRIBUTES_SHARED_SOURCES} kolabconvertertest.cpp)

target_link_libraries(kolabconvertertest ${KDEPIMLIBS_AKONADI_LIBS} ${QT_QTXML_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KIO_LIBS} ${KDEPIMLIBS_KABC_LIBS} ${KDEPIMLIBS_AKONADI_KMIME_LIBS} ${KDEPIMLIBS_KMIME_LIBS} ${QT_QTTEST_LIBRARY}  kdepim-copy ${KDEPIMLIBS_KCALCORE_LIBS})


#akonadi_add_resourcetest( imap-kolab imaptest-kolab.es )
#akonadi_add_resourcetest( imap-dovecot imaptest-dovecot.es )
#akonadi_add_resourcetest( kolab kolabtest.es )

