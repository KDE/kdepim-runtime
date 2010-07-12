set(akonadi_qml_test_srcs
  qmltest.cpp
)

kde4_add_executable(akonadi_qml_test ${akonadi_qml_test_srcs} )

target_link_libraries(akonadi_qml_test 
    ${QT_QTDECLARATIVE_LIBRARY}
    ${KDEPIMLIBS_AKONADI_LIBS}
)

