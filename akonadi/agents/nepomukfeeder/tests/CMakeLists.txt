
kde4_add_unit_test( sparqlbuildertest TESTNAME sparqlbuilder sparqlbuildertest.cpp ${NEPOMUKFEEDER_SHARED_SRCS} )
target_link_libraries( sparqlbuildertest ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${KDE4_AKONADI_LIBS} ${KDE4_NEPOMUK_LIBS} ${STRIGI_STREAMANALYZER_LIBRARY} niefast)
