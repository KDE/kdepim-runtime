
set(testkincidencechooser_SRCS testkincidencechooser.cpp )

kde4_add_executable(testkincidencechooser TEST ${testkincidencechooser_SRCS})

target_link_libraries(testkincidencechooser kdepim-copy ${KDEPIMLIBS_KCAL_LIBRARY} ${KDE4_KDEUI_LIBS} )
