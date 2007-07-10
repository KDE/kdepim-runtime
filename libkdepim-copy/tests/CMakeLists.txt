
########### next target ###############

set(testwizard_SRCS testwizard.cpp )


kde4_add_kcfg_files(testwizard_SRCS myconfig.kcfgc )


kde4_add_test_executable(testwizard ${testwizard_SRCS})

target_link_libraries(testwizard  kdepim ${KDE4_KDECORE_LIBS} )


########### next target ###############

set(testaddresseelineedit_SRCS testaddresseelineedit.cpp )



kde4_add_test_executable(testaddresseelineedit ${testaddresseelineedit_SRCS})

target_link_libraries(testaddresseelineedit  kdepim ${KDE4_KDECORE_LIBS} )


########### next target ###############

set(testaddresseeselector_SRCS testaddresseeselector.cpp )



kde4_add_test_executable(testaddresseeselector ${testaddresseeselector_SRCS})

target_link_libraries(testaddresseeselector  kdepim ${KDE4_KDECORE_LIBS} )


########### next target ###############

set(testutf7decoder_SRCS testutf7decoder.cpp )



kde4_add_test_executable(testutf7decoder ${testutf7decoder_SRCS})

target_link_libraries(testutf7decoder  kdepim ${KDE4_KDECORE_LIBS} )


########### next target ###############

set(testutf7encoder_SRCS testutf7encoder2.cpp )



kde4_add_test_executable(testutf7encoder ${testutf7encoder_SRCS})

target_link_libraries(testutf7encoder  kdepim ${KDE4_KDECORE_LIBS} )


########### next target ###############

set(testdateedit_SRCS testdateedit.cpp )


kde4_add_test_executable(testdateedit ${testdateedit_SRCS})

target_link_libraries(testdateedit  kdepim ${KDE4_KDECORE_LIBS} )

########### next target ###############

set(testdistrlist_SRCS testdistrlist.cpp )


kde4_add_test_executable(testdistrlist ${testdistrlist_SRCS})

target_link_libraries(testdistrlist  kdepim ${KDE4_KDECORE_LIBS} )

########### next target ###############

set(testldapclient_SRCS testldapclient.cpp )


kde4_add_test_executable(testldapclient ${testldapclient_SRCS})

target_link_libraries(testldapclient  kdepim ${KDE4_KDECORE_LIBS} )

########### next target ###############

set(testkmeditor_SRCS testkmeditor.cpp )


kde4_add_test_executable(testkmeditor ${testkmeditor_SRCS})

target_link_libraries(testkmeditor  kdepim ${KDE4_KDEUI_LIBS} )



########### next target ###############

set(testkmeditorwin_SRCS testkmeditorwin.cpp )


kde4_add_test_executable(testkmeditorwin ${testkmeditorwin_SRCS})

target_link_libraries(testkmeditorwin  kdepim ${KDE4_KDEUI_LIBS} )


