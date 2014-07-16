include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )
add_definitions (-DAUTOCONFIG_DATA_DIR="\\"${CMAKE_CURRENT_SOURCE_DIR}/data\\"")
add_definitions (-DCURRENT_SOURCE_DIR="\\"${CMAKE_CURRENT_SOURCE_DIR}\\"")

kde4_add_unit_test(ispdbtest ispdbtest.cpp ../ispdb.cpp)

target_link_libraries(ispdbtest
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDEPIMLIBS_KMIME_LIBS}
    ${KDEPIMLIBS_KLDAP_LIBS}
    ${QT_QTTEST_LIBRARY}
  )

kde4_add_unit_test(autoconfigkolabmailtest autoconfigkolabmailtest.cpp ../ispdb.cpp ../autoconfigkolabmail.cpp)

target_link_libraries(autoconfigkolabmailtest
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDEPIMLIBS_KMIME_LIBS}
    ${KDEPIMLIBS_KLDAP_LIBS}
    ${QT_QTTEST_LIBRARY}
  )
  
kde4_add_unit_test(autoconfigkolabldaptest autoconfigkolabldaptest.cpp ../ispdb.cpp ../autoconfigkolabmail.cpp ../autoconfigkolabldap.cpp)

target_link_libraries(autoconfigkolabldaptest
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDEPIMLIBS_KMIME_LIBS}
    ${KDEPIMLIBS_KLDAP_LIBS}
    ${QT_QTTEST_LIBRARY}
  )
  
kde4_add_unit_test(autoconfigkolabfreebusytest autoconfigkolabfreebusy.cpp ../ispdb.cpp ../autoconfigkolabmail.cpp ../autoconfigkolabfreebusy.cpp)

target_link_libraries(autoconfigkolabfreebusytest
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KIO_LIBS}
    ${KDEPIMLIBS_KMIME_LIBS}
    ${KDEPIMLIBS_KLDAP_LIBS}
    ${QT_QTTEST_LIBRARY}
  )