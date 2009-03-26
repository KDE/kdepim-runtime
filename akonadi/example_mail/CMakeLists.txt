project(example_mail)

set(example_mail_SRCS
    main.cpp
    mailwidget.cpp
    mailmodel.cpp
    mailcomposer.cpp
    mainwindow.cpp
    #    modeltest.cpp
    ../example_contacts/contactsmodel.cpp
    ../akonadi_next/tests/dynamictreemodel.cpp
)

kde4_add_executable(example_mail ${example_mail_SRCS} )

target_link_libraries(example_mail
                      akonadi_next
                      ${KDE4_AKONADI_LIBS}
                      ${KDE4_KIO_LIBS}
                      ${KDE4_KABC_LIBS}
                      ${KDE4_KMIME_LIBS}
)

install(TARGETS example_mail
        DESTINATION ${BIN_INSTALL_DIR})

install(FILES
  example_mailui.rc
  DESTINATION  ${DATA_INSTALL_DIR}/example_mail)





