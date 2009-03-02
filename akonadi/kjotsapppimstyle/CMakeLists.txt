project(kjotsrewriteapppimstyle)

set(kjotsrewriteapppimstyle_SRCS
    main.cpp
    kjotsrewritepimstyle.cpp
)

kde4_add_executable(kjotsrewriteapppimstyle ${kjotsrewriteapppimstyle_SRCS} )

target_link_libraries(kjotsrewriteapppimstyle
                      kjots_common
                      ${KDE4_KIO_LIBS}
)

install(TARGETS kjotsrewriteapppimstyle
        DESTINATION ${BIN_INSTALL_DIR})

