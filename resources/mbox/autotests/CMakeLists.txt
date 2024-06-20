
set(mbox_deleteitemsattributetest_source deleteitemsattributetest.cpp deleteitemsattributetest.h ../deleteditemsattribute.cpp)
add_executable(deleteitemsattributetest ${mbox_deleteitemsattributetest_source})
add_test(NAME deleteitemsattributetest COMMAND deleteitemsattributetest)
ecm_mark_as_test(deleteitemsattributetest)
target_link_libraries(deleteitemsattributetest Qt::Test Qt::Gui KPim6::AkonadiCore KPim6::Mbox)

