kde4_header()

include_directories( ${KDE4_INCLUDES} )


########### next target ###############

set(testwizard_SRCS testwizard.cpp )

kde4_automoc(${testwizard_SRCS})

kde4_add_kcfg_files(testwizard_SRCS myconfig.kcfgc )

if(KDE4_BUILD_TESTS)

kde4_add_executable(testwizard ${testwizard_SRCS})

target_link_libraries(testwizard  kdepim ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### next target ###############

set(testaddresseelineedit_SRCS testaddresseelineedit.cpp )

kde4_automoc(${testaddresseelineedit_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(testaddresseelineedit ${testaddresseelineedit_SRCS})

target_link_libraries(testaddresseelineedit  kdepim ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### next target ###############

set(testaddresseeselector_SRCS testaddresseeselector.cpp )

kde4_automoc(${testaddresseeselector_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(testaddresseeselector ${testaddresseeselector_SRCS})

target_link_libraries(testaddresseeselector  kdepim ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### next target ###############

set(testutf7decoder_SRCS testutf7decoder.cpp )

kde4_automoc(${testutf7decoder_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(testutf7decoder ${testutf7decoder_SRCS})

target_link_libraries(testutf7decoder  kdepim ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### next target ###############

set(testutf7encoder_SRCS testutf7encoder2.cpp )

kde4_automoc(${testutf7encoder_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(testutf7encoder ${testutf7encoder_SRCS})

target_link_libraries(testutf7encoder  kdepim ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### next target ###############

set(testdateedit_SRCS testdateedit.cpp )

kde4_automoc(${testdateedit_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(testdateedit ${testdateedit_SRCS})

target_link_libraries(testdateedit  kdepim ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### next target ###############

set(testlinklocator_SRCS testlinklocator.cpp )

kde4_automoc(${testlinklocator_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(testlinklocator ${testlinklocator_SRCS})

target_link_libraries(testlinklocator  kdepim ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### next target ###############

set(testdistrlist_SRCS testdistrlist.cpp )

kde4_automoc(${testdistrlist_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(testdistrlist ${testdistrlist_SRCS})

target_link_libraries(testdistrlist  kdepim ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### next target ###############

set(testldapclient_SRCS testldapclient.cpp )

kde4_automoc(${testldapclient_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(testldapclient ${testldapclient_SRCS})

target_link_libraries(testldapclient  kdepim ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(top_builddir)/libkdepim -I$(top_srcdir)/libemailfunctions $(all_includes)
#AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#LDADD = ../libkdepim.la $(LIB_KDECORE)
#
#check_PROGRAMS = testwizard testaddresseelineedit \
#                 testaddresseeselector \
#                 testutf7decoder \
#                 testutf7encoder \
#                 test_kregexp \
#                 testdateedit \
#                 testlinklocator \
#                 testdistrlist \
#                 testldapclient
#
#
#testwizard_SOURCES = testwizard.cpp myconfig.kcfgc
#testaddresseelineedit_SOURCES = testaddresseelineedit.cpp
#testaddresseeselector_SOURCES = testaddresseeselector.cpp
#testutf7encoder_SOURCES = testutf7encoder2.cpp
#testutf7decoder_SOURCES = testutf7decoder.cpp
#test_kregexp_SOURCES = test_kregexp.cpp
#testdateedit_SOURCES = testdateedit.cpp
#testlinklocator_SOURCES = testlinklocator.cpp
#testdistrlist_SOURCES = testdistrlist.cpp
#testldapclient_SOURCES = testldapclient.cpp
#
#TESTS = testdistrlist
#
#METASOURCES = AUTO
#
##example_LDFLAGS = $(all_libraries) $(KDE_RPATH)
##example_LDADD = ../libkdepim.la $(LIB_KDECORE)
##example_SOURCES = example.cpp exampleprefs_base.kcfgc
#
