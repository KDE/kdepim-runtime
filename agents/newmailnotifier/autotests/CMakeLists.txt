
set( newmailnotifierattribute_source newmailnotifierattributetest.cpp ../newmailnotifierattribute.cpp)
kde4_add_unit_test( newmailnotifierattributetest ${newmailnotifierattribute_source})
target_link_libraries( newmailnotifierattributetest ${QT_QTTEST_LIBRARY} ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${KDEPIMLIBS_AKONADI_LIBS})
