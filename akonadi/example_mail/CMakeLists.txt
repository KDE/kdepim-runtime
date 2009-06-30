project(example_mail)

set(example_mail_SRCS
    main.cpp
    mailwidget.cpp
    #    mailmodel.cpp
    mailcomposer.cpp
    mainwindow.cpp
    #    modeltest.cpp
    #    ../example_contacts/contactsmodel.cpp
    #   ../akonadi_next/tests/dynamictreemodel.cpp
)

include_directories( ../akonaditemplateloader )

kde4_add_executable(example_mail ${example_mail_SRCS} )

target_link_libraries(example_mail
                      akonadi_next
                      akonadi_template_loader
                      ${KDEPIMLIBS_AKONADI_LIBS}
                      ${KDE4_KIO_LIBS}
                      ${KDEPIMLIBS_KABC_LIBS}
                      ${KDEPIMLIBS_KMIME_LIBS}
)

install(TARGETS example_mail
        DESTINATION ${BIN_INSTALL_DIR})

install(FILES
  example_mailui.rc
  DESTINATION  ${DATA_INSTALL_DIR}/example_mail)





