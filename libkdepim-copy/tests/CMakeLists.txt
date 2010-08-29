
set(testkincidencechooser_SRCS testkincidencechooser.cpp )

kde4_add_executable(testkincidencechooser TEST ${testkincidencechooser_SRCS})

target_link_libraries(testkincidencechooser kdepim-copy ${KDE4_KDEUI_LIBS} ${KDEPIMLIBS_KCALCORE_LIBS} )
