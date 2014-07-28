
set(ispdb_srcs
  main.cpp
  ispdb.cpp
)

add_executable(ispdb ${ispdb_srcs})

target_link_libraries(ispdb
  KF5::I18n  
  KF5::KIOCore
  KF5::Mime
  Qt5::Widgets
  Qt5::Xml
)

install(TARGETS ispdb ${INSTALL_TARGETS_DEFAULT_ARGS})
