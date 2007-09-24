set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )




########### next target ###############

set(test_SRCS test.cpp )


kde4_add_executable(test TEST ${test_SRCS})

target_link_libraries(test  ${KDE4_KDECORE_LIBS} )

########### install files ###############





#original Makefile.am contents follow:

#AM_CPPFLAGS = -I$(top_srcdir) -I../core $(all_includes)
#LDADD = $(LIB_KPARTS) ../core/libkomposer.la
#AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#
#check_PROGRAMS = test
#
#test_SOURCES = test.cpp
