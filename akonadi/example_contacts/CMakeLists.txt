project(example_contacts)

set(example_contacts_SRCS
    main.cpp
    contactswidget.cpp
    #    contactsmodel.cpp
    mainwindow.cpp
)

kde4_add_executable(example_contacts ${example_contacts_SRCS} )

target_link_libraries(example_contacts
                      akonadi_next
                      ${KDEPIMLIBS_AKONADI_LIBS}
                      ${KDE4_KIO_LIBS}
                      ${KDEPIMLIBS_KABC_LIBS}
)

install(TARGETS example_contacts
        DESTINATION ${BIN_INSTALL_DIR})

