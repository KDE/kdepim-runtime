project(akonadi_next)

if(CMAKE_SOURCE_DIR STREQUAL "${akonadi_next_SOURCE_DIR}")
  find_package(KDE4 REQUIRED)
  include(KDE4Defaults)
  find_package(Akonadi REQUIRED)
  find_package(KdepimLibs REQUIRED)
endif(CMAKE_SOURCE_DIR STREQUAL "${akonadi_next_SOURCE_DIR}")

include_directories(
  ${AKONADI_INCLUDE_DIR}
  ${KDE4_INCLUDES}
  ${KDEPIMLIBS_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${Boost_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")

set( akonadi_next_SRCS
  note.cpp
  notecreatorandselector.cpp
  entityorderproxymodel.cpp
  entityrightsfiltermodel.cpp
  collectionchildorderattribute.cpp
  collectionselectionproxymodel.cpp
  modeltest.cpp
  quotacolorproxymodel.cpp
  checkableitemproxymodel.cpp
  incidenceattachmentmodel.cpp
  # TODO Move this to akonadi-kabc
  contactsmodel.cpp
  # TODO Move this to akonadi-mail?
  mailmodel.cpp
  amazingcompleter.cpp
  amazingdelegate.cpp
  modelstatesaver.cpp
  entitymodelstatesaver.cpp
  recursivecollectionfilterproxymodel.cpp
)

kde4_add_library(akonadi_next ${LIBRARY_TYPE} ${akonadi_next_SRCS} )

target_link_libraries(akonadi_next
                      ${AKONADI_COMMON_LIBRARIES}
                      ${QT_QTCORE_LIBRARY}
                      ${QT_QTGUI_LIBRARY}
                      ${KDEPIMLIBS_AKONADI_LIBS}
                      ${KDE4_KIO_LIBS}
                      ${KDEPIMLIBS_KABC_LIBS}
                      ${KDEPIMLIBS_KMIME_LIBS}
                      ${KDEPIMLIBS_KCALCORE_LIBS}
)

set_target_properties(akonadi_next PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )

install(TARGETS akonadi_next ${INSTALL_TARGETS_DEFAULT_ARGS})

