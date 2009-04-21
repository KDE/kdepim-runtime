project(akonadi_next)

find_package(KDE4 REQUIRED)
find_package(Akonadi REQUIRED)
find_package(KdepimLibs REQUIRED)

include_directories(
  ${AKONADI_INCLUDE_DIR}
  ${KDE4_INCLUDES}
  ${KDEPIMLIBS_INCLUDE_DIR}
  ${QT_INCLUDES}
  ${Boost_INCLUDE_DIR}
)

include(KDE4Defaults)

set( akonadi_next_SRCS
  abstractitemmodel.cpp
  abstractitemmodel_p.cpp
  abstractproxymodel.cpp
#   qsortfilterproxymodel.cpp
  entitytreemodel.cpp
  entitytreemodel_p.cpp
  descendantentitiesproxymodel.cpp
  entityfilterproxymodel.cpp
  entitytreeview.cpp
  collectionchildorderattribute.cpp
  modeltest.cpp
  emaillineedit.cpp
  # TODO Move this to akonadi-kabc
  contactsmodel.cpp
  # TODO Move this to akonadi-mail?
  mailmodel.cpp
  entityupdateadapter.cpp
  )

kde4_add_library(akonadi_next SHARED ${akonadi_next_SRCS} )

target_link_libraries(akonadi_next
                      ${AKONADI_COMMON_LIBRARIES}
                      ${QT_QTCORE_LIBRARY}
                      ${QT_QTGUI_LIBRARY}
                      ${KDE4_AKONADI_LIBS}
                      ${KDE4_KIO_LIBS}
                      ${KDE4_KABC_LIBS}
                      ${KDE4_KMIME_LIBS}
)

set_target_properties(akonadi_next PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )

install(FILES
#   abstractitemmodel.h
  collectionchildorderattribute.h
  descendantentitiesproxymodel.h
  entityfilterproxymodel.h
  entitytreemodel.h
  entitytreeview.h
  entityupdateadapter.h
  akonadi_next_export.h
  DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel
)

install(TARGETS akonadi_next ${INSTALL_TARGETS_DEFAULT_ARGS})

add_subdirectory(tests)
