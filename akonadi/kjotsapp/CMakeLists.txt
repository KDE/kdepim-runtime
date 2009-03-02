project(kjotsrewriteapp)

set(kjotsrewriteapp_SRCS
    main.cpp
    kjotsrewrite.cpp
)

kde4_add_executable(kjotsrewriteapp ${kjotsrewriteapp_SRCS} )

target_link_libraries(kjotsrewriteapp
                      kjots_common
                      ${KDE4_KIO_LIBS}
)

install(TARGETS kjotsrewriteapp
        DESTINATION ${BIN_INSTALL_DIR})
