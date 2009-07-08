project(proxymodeltestapp)

include_directories( ${KDE4_INCLUDES}
  ${PROJECT_BINARY_DIR}
  ${PROJECT_BINARY_DIR}/..
  ${PROJECT_SOURCE_DIR}/..
  ${PROJECT_BINARY_DIR}/../tests
  ${PROJECT_SOURCE_DIR}/../tests
)

add_definitions(
	-DAKONADI_NEXT_EXPORT=
)
set(proxymodeltestapp_SRCS
    main.cpp
    mainwindow.cpp
    ../tests/dynamictreemodel.cpp
    ../abstractitemmodel.cpp
    ../abstractitemmodel_p.cpp
    ../abstractproxymodel.cpp
)

kde4_add_executable(proxymodeltestapp ${proxymodeltestapp_SRCS} )

target_link_libraries(proxymodeltestapp
                      akonadi_next
                      ${KDEPIMLIBS_AKONADI_LIBS}
                      ${KDE4_KIO_LIBS}
                      ${KDEPIMLIBS_KABC_LIBS}
)


set(statisticsmodeltestapp_SRCS
    statisticsmodeltestapp.cpp
)

kde4_add_executable(statisticsmodeltestapp ${statisticsmodeltestapp_SRCS} )

target_link_libraries(statisticsmodeltestapp
                      akonadi_next
                      ${KDEPIMLIBS_AKONADI_LIBS}
                      ${KDE4_KIO_LIBS}
                      ${KDEPIMLIBS_KABC_LIBS}
)

