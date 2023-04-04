add_executable(kolabformatchecker kolabformatchecker.cpp)
target_link_libraries(kolabformatchecker pimkolab Qt::Core KPim6::AkonadiCore KF6::Contacts KF6::CalendarCore KPim6::Mime)
