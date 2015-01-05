
set( mbox_deleteitemsattributetest_source deleteitemsattributetest.cpp ../deleteditemsattribute.cpp)
kde4_add_unit_test( deleteitemsattributetest ${mbox_deleteitemsattributetest_source})
target_link_libraries( deleteitemsattributetest ${QT_QTTEST_LIBRARY} ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_KMBOX_LIBS})

