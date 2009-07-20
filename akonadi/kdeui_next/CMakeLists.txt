project(kdeui_next)

find_package(KDE4 REQUIRED)

include_directories(
  ${KDE4_INCLUDES}
  ${QT_INCLUDES}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

include(KDE4Defaults)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

set( kdeui_next_SRCS
  kdescendantentitiesproxymodel.cpp
  kselectionproxymodel.cpp
)

kde4_add_library(kdeui_next SHARED ${kdeui_next_SRCS} )

target_link_libraries(kdeui_next
                      ${QT_QTCORE_LIBRARY}
                      ${QT_QTGUI_LIBRARY}
                      ${KDE4_KDEUI_LIBS}
)

set_target_properties(kdeui_next PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )

install(TARGETS kdeui_next ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
add_subdirectory(testapp)
