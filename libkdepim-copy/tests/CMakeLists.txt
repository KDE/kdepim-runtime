
set(testkincidencechooser_SRCS testkincidencechooser.cpp )

kde4_add_executable(testkincidencechooser TEST ${testkincidencechooser_SRCS})

target_link_libraries(testkincidencechooser kdepim-copy ${KDEPIMLIBS_KCAL_LIBS} ${KDE4_KDEUI_LIBS} ${KDEPIMLIBS_KCALCORE_LIBS} )
