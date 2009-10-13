FIND_PACKAGE(Akonadi REQUIRED)
FIND_PACKAGE(KdepimLibs REQUIRED)

SET(akinotes_SRCS
   main.cpp
   akinotes.cpp
   akinoteitem.cpp
 )

#KDE4_ADD_KCFG_FILES(akinotes_SRCS settings.kcfgc)

KDE4_ADD_EXECUTABLE(akinotes ${akinotes_SRCS})

TARGET_LINK_LIBRARIES(akinotes	${KDE4_KDEUI_LIBS}
				${KDE4_AKONADI_KMIME_LIBS}
				${KDE4_AKONADI_LIBS}
				${KDEPIMLIBS_KCAL_LIBS}
				stickynotes
				stickynotes-standalone)

INSTALL(TARGETS akinotes ${INSTALL_TARGETS_DEFAULT_ARGS})

#INSTALL(FILES akinotes.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
#INSTALL(FILES akinotes.kcfg DESTINATION  ${KCFG_INSTALL_DIR})
#INSTALL(FILES akinotesui.rc DESTINATION ${DATA_INSTALL_DIR}/akinotes)

