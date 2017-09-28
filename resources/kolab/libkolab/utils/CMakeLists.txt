
find_package(Boost REQUIRED COMPONENTS program_options)

add_executable(kolabformatchecker kolabformatchecker.cpp)
target_link_libraries(kolabformatchecker kolab ${Boost_LIBRARIES})
