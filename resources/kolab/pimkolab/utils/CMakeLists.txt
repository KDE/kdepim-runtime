
find_package(Boost REQUIRED COMPONENTS program_options)

add_executable(kolabformatchecker kolabformatchecker.cpp)
target_link_libraries(kolabformatchecker pimkolab ${Boost_LIBRARIES} Qt5::Core KF5::AkonadiCore KF5::Contacts KF5::CalendarCore KF5::Mime)
