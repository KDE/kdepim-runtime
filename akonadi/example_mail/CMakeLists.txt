project(example_mail)

set(example_mail_SRCS
    main.cpp
    mailwidget.cpp
    mainwindow.cpp
)

kde4_add_executable(example_mail ${example_mail_SRCS} )

target_link_libraries(example_mail
                      akonadi_next
                      ${KDE4_AKONADI_LIBS}
                      ${KDE4_KIO_LIBS}
)

install(TARGETS example_mail
        DESTINATION ${BIN_INSTALL_DIR})

