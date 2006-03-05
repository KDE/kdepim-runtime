kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kdepimwidgets/pics FILES addresseelineedit.png                           clicklineedit.png  kdateedit.png  ktimeedit.png )

kde4_footer()



#original Makefile.am contents follow:

#kdepimwidgetsdata_DATA = addresseelineedit.png \
#                         clicklineedit.png  kdateedit.png  ktimeedit.png
#
#kdepimwidgetsdatadir = $(kde_datadir)/kdepimwidgets/pics
#
#EXTRA_DIST = $(kdepimwidgetsdata_DATA)
