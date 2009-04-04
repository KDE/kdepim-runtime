project(akonaditests)

ENABLE_TESTING()

FIND_PACKAGE(KDE4 REQUIRED)
INCLUDE_DIRECTORIES( ${KDE4_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} )

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

SET(testSources
  dynamictreemodel.cpp
  ../modeltest.cpp
  ../descendantentitiesproxymodel.cpp
  ../abstractproxymodel.cpp
  ../abstractitemmodel_p.cpp
  ../abstractitemmodel.cpp
)


MACRO(KDEUI_UNIT_TESTS)
       FOREACH(_testname ${ARGN})
               kde4_add_unit_test(${_testname} ${_testname}.cpp ${testSources} )
               target_link_libraries(${_testname} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} ${QT_QTXML_LIBRARY} ${KDEWIN32_LIBRARIES} )
       ENDFOREACH(_testname)
ENDMACRO(KDEUI_UNIT_TESTS)

KDEUI_UNIT_TESTS(
  descendantentitiesproxymodeltest
)



