set(ispdb_srcs
  main.cpp
  ispdb.cpp
  autoconfigkolabmail.cpp
  autoconfigkolabldap.cpp
  autoconfigkolabfreebusy.cpp
)

kde4_add_executable(ispdb ${ispdb_srcs})

target_link_libraries(ispdb
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KIO_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
)

install(TARGETS ispdb ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)