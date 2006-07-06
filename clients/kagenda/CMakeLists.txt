
set(kagenda_SRCS 
  agendaview.cpp main.cpp stripeview.cpp qwt_abstract_slider.cpp
  qwt_double_range.cpp qwt_math.cpp qwt_wheel.cpp agendamodel.cpp
  dummymodel.cpp contactmodel.cpp dataprovider.cpp
)

kde4_automoc(${kagenda_SRCS})

kde4_add_executable(kagenda ${kagenda_SRCS})

target_link_libraries(kagenda kcal akonadi ${QT_QTGUI_LIBRARY})

install_targets(/bin kagenda )
