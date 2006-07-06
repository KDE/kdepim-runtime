TEMPLATE = app
TARGET += 
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += agendaview.h stripeview.h qwt_abstract_slider.h qwt_double_range.h \
  qwt_math.h qwt_wheel.h agendamodel.h event.h
SOURCES += agendaview.cpp main.cpp stripeview.cpp qwt_abstract_slider.cpp \
  qwt_double_range.cpp qwt_math.cpp qwt_wheel.cpp agendamodel.cpp \
  dummymodel.cpp contactmodel.cpp

