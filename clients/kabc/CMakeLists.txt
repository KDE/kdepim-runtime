include_directories( ${kdepim-runtime_SOURCE_DIR} )

set(kabcviewer_bin_SRCS
  kabcviewer.cpp
)


# todo: more appropriate icon?
kde4_add_app_icon(kabcviewer_bin_SRCS "${KDE4_ICON_DIR}/oxygen/*/apps/office-address-book.png")

kde4_add_executable(kabcviewer_bin ${kabcviewer_bin_SRCS})
set_target_properties(kabcviewer_bin PROPERTIES OUTPUT_NAME kabcviewer)

target_link_libraries( kabcviewer_bin ${KDEPIMLIBS_AKONADI_LIBS} akonadi-kabccommon ${KDE4_KDEUI_LIBS} )

#install(TARGETS kabcviewer_bin ${INSTALL_TARGETS_DEFAULT_ARGS} )

########## next target #######################

set(kabceditor_bin_SRCS
  kabceditor.cpp
)

# todo: more appropriate icon?
kde4_add_app_icon(kabceditor_bin_SRCS "${KDE4_ICON_DIR}/oxygen/*/apps/office-address-book.png")

kde4_add_executable(kabceditor_bin ${kabceditor_bin_SRCS})
set_target_properties(kabceditor_bin PROPERTIES OUTPUT_NAME kabceditor)

target_link_libraries( kabceditor_bin ${KDEPIMLIBS_AKONADI_LIBS} akonadi-kabccommon ${KDE4_KDEUI_LIBS} )

#install(TARGETS kabceditor_bin ${INSTALL_TARGETS_DEFAULT_ARGS} )
