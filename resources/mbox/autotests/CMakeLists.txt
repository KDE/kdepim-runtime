
set( mbox_deleteitemsattributetest_source deleteitemsattributetest.cpp ../deleteditemsattribute.cpp)
add_executable( deleteitemsattributetest ${mbox_deleteitemsattributetest_source})
add_test(NAME deleteitemsattributetest COMMAND deleteitemsattributetest)
ecm_mark_as_test(deleteitemsattributetest)
target_link_libraries( deleteitemsattributetest Qt5::Test Qt5::Gui KF5::AkonadiCore KF5::Mbox)

