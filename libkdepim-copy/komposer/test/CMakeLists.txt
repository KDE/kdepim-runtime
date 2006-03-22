kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(test_SRCS test.cpp )

kde4_automoc(${test_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(test ${test_SRCS})

target_link_libraries(test  ${KDE4_KDECORE_LIBS} )

endif(KDE4_BUILD_TESTS)

########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(top_srcdir) -I../core $(all_includes)
#LDADD = $(LIB_KPARTS) ../core/libkomposer.la
#AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#
#check_PROGRAMS = test
#
#test_SOURCES = test.cpp
