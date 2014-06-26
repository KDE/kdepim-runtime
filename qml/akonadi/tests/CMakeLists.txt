set(akonadi_qml_test_srcs
  qmltest.cpp
)

add_executable(akonadi_qml_test ${akonadi_qml_test_srcs} )

target_link_libraries(akonadi_qml_test 
    Qt5::Declarative
    KF5::AkonadiCore
)

