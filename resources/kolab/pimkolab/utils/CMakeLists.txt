add_executable(kolabformatchecker kolabformatchecker.cpp)
target_link_libraries(kolabformatchecker pimkolab Qt::Core KF5::AkonadiCore KF5::Contacts KF5::CalendarCore KF5::Mime)
