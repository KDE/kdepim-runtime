include_directories( ${CMAKE_SOURCE_DIR}/akonadi)

set(kagenda_SRCS 
  agendaview.cpp main.cpp stripeview.cpp qwt_abstract_slider.cpp
  qwt_double_range.cpp qwt_math.cpp qwt_wheel.cpp agendamodel.cpp
  dummymodel.cpp contactmodel.cpp dataprovider.cpp
)


kde4_add_executable(kagenda ${kagenda_SRCS})

target_link_libraries(kagenda ${KDE4_KDEUI_LIBS} ${KDE4_KCAL_LIBS} akonadicore ${QT_QTGUI_LIBRARY})

install(TARGETS kagenda DESTINATION ${BIN_INSTALL_DIR})
