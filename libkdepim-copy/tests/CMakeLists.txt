if(KDE4_BUILD_TESTS)



########### next target ###############

set(testwizard_SRCS testwizard.cpp )

kde4_automoc(${testwizard_SRCS})

kde4_add_kcfg_files(testwizard_SRCS myconfig.kcfgc )


kde4_add_executable(testwizard ${testwizard_SRCS})

target_link_libraries(testwizard  kdepim ${KDE4_KDECORE_LIBS} )


########### next target ###############

set(testaddresseelineedit_SRCS testaddresseelineedit.cpp )

kde4_automoc(${testaddresseelineedit_SRCS})


kde4_add_executable(testaddresseelineedit ${testaddresseelineedit_SRCS})

target_link_libraries(testaddresseelineedit  kdepim ${KDE4_KDECORE_LIBS} )


########### next target ###############

set(testaddresseeselector_SRCS testaddresseeselector.cpp )

kde4_automoc(${testaddresseeselector_SRCS})


kde4_add_executable(testaddresseeselector ${testaddresseeselector_SRCS})

target_link_libraries(testaddresseeselector  kdepim ${KDE4_KDECORE_LIBS} )


########### next target ###############

set(testutf7decoder_SRCS testutf7decoder.cpp )

kde4_automoc(${testutf7decoder_SRCS})


kde4_add_executable(testutf7decoder ${testutf7decoder_SRCS})

target_link_libraries(testutf7decoder  kdepim ${KDE4_KDECORE_LIBS} )


########### next target ###############

set(testutf7encoder_SRCS testutf7encoder2.cpp )

kde4_automoc(${testutf7encoder_SRCS})


kde4_add_executable(testutf7encoder ${testutf7encoder_SRCS})

target_link_libraries(testutf7encoder  kdepim ${KDE4_KDECORE_LIBS} )


########### next target ###############

set(testdateedit_SRCS testdateedit.cpp )

kde4_automoc(${testdateedit_SRCS})

kde4_add_executable(testdateedit ${testdateedit_SRCS})

target_link_libraries(testdateedit  kdepim ${KDE4_KDECORE_LIBS} )

########### next target ###############

set(testlinklocator_SRCS testlinklocator.cpp )

kde4_automoc(${testlinklocator_SRCS})

kde4_add_executable(testlinklocator ${testlinklocator_SRCS})

target_link_libraries(testlinklocator  kdepim ${KDE4_KDECORE_LIBS} )

########### next target ###############

set(testdistrlist_SRCS testdistrlist.cpp )

kde4_automoc(${testdistrlist_SRCS})

kde4_add_executable(testdistrlist ${testdistrlist_SRCS})

target_link_libraries(testdistrlist  kdepim ${KDE4_KDECORE_LIBS} )

########### next target ###############

set(testldapclient_SRCS testldapclient.cpp )

kde4_automoc(${testldapclient_SRCS})

kde4_add_executable(testldapclient ${testldapclient_SRCS})

target_link_libraries(testldapclient  kdepim ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

