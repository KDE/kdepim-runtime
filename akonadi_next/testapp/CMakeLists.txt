project(proxymodeltestapp)

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


