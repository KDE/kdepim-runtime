
set( mbox_deleteitemsattributetest_source deleteitemsattributetest.cpp ../deleteditemsattribute.cpp)
add_executable( deleteitemsattributetest ${mbox_deleteitemsattributetest_source})
add_test(deleteitemsattributetest deleteitemsattributetest)
ecm_mark_as_test(deleteitemsattributetest)
target_link_libraries( deleteitemsattributetest Qt5::Test KF5::KDELibs4Support Qt5::Gui KF5::AkonadiCore KF5::Mbox)

