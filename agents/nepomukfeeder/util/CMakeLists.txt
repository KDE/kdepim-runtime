include_directories(
        ../
)

set(shared_SRCS ../feederqueue.cpp ../itemqueue.cpp ../nepomukhelpers.cpp ../pluginloader.cpp ../propertycache.cpp)

set(nepomukpimindexerutility_SRCS
   nepomukpimindexerutility.cpp
   indexhelpermodel.cpp
   main.cpp
)
kde4_add_ui_files(nepomukpimindexerutility_SRCS nepomukpimindexerutility.ui)
kde4_add_executable(nepomukpimindexerutility ${nepomukpimindexerutility_SRCS} ${shared_SRCS})
target_link_libraries( nepomukpimindexerutility
  ${NEPOMUKFEEDER_SHARED_LIBS}
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
    akonadi-kde
    ${QT_QTGUI_LIBRARY}
    nepomukfeederpluginlib
    nepomukquery
    nepomuk
    ${NEPOMUK_CORE_LIBRARY}
)

install(TARGETS nepomukpimindexerutility DESTINATION ${BIN_INSTALL_DIR})
install(FILES nepomukpimindexerutility.rc
        DESTINATION  ${DATA_INSTALL_DIR}/nepomukpimindexerutility)
