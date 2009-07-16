project(kdeui_next)

find_package(KDE4 REQUIRED)
find_package(Akonadi REQUIRED)
find_package(KdepimLibs REQUIRED)

include_directories(
  ${AKONADI_INCLUDE_DIR}
  ${KDE4_INCLUDES}
  ${KDEPIMLIBS_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${Boost_INCLUDE_DIR}
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
                      ${AKONADI_COMMON_LIBRARIES}
                      ${QT_QTCORE_LIBRARY}
                      ${QT_QTGUI_LIBRARY}
                      ${KDEPIMLIBS_AKONADI_LIBS}
                      ${KDE4_KIO_LIBS}
                      ${KDEPIMLIBS_KABC_LIBS}
                      ${KDEPIMLIBS_KMIME_LIBS}
)

set_target_properties(kdeui_next PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )

install(TARGETS kdeui_next ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
add_subdirectory(testapp)
