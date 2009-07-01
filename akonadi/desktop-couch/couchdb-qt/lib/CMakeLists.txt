include_directories (./)

set ( COUCHDBQT_LIB_MAJOR_VERSION 1 )
set ( COUCHDBQT_LIB_MINOR_VERSION 0 )
set ( COUCHDBQT_LIB_PATCH_VERSION 0 )

install(FILES
        couchdb-qt.h
        DESTINATION include/couchdbqt
)

set(COUCHDBQT_MOC_HDRS
    couchdb-qt.h
)

set(CMAKE_INSTALL_NAME_DIR ${LIB_INSTALL_DIR})

qt4_wrap_cpp(COUCHDBQT_MOC_SRCS ${COUCHDBQT_MOC_HDRS})

set (COUCHDBQT_SRCS couchdb-qt.cpp)

add_library (couchdb-qt SHARED ${COUCHDBQT_SRCS} ${COUCHDBQT_MOC_SRCS})
target_link_libraries( couchdb-qt ${QT_LIBRARIES} ${QJSON_LIBRARIES})

set_target_properties(couchdb-qt PROPERTIES
                      VERSION ${COUCHDBQT_LIB_MAJOR_VERSION}.${COUCHDBQT_LIB_MINOR_VERSION}.${COUCHDBQT_LIB_PATCH_VERSION}
                      SOVERSION ${COUCHDBQT_LIB_MAJOR_VERSION}
                      DEFINE_SYMBOL COUCHDBQT_MAKEDLL
                      )

install(TARGETS couchdb-qt EXPORT  LIBRARY DESTINATION ${LIB_INSTALL_DIR} ${INSTALL_TARGETS_DEFAULT_ARGS})

