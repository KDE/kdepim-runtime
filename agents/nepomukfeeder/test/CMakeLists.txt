include_directories(
        ../
)

add_definitions(-DTEST_DATA_PATH=\\"${CMAKE_CURRENT_SOURCE_DIR}\\")

kde4_add_executable(akonadinepomukfeeder_indexer akonadinepomukfeeder_indexer.cpp ../feederqueue.cpp ../itemqueue.cpp ../propertycache.cpp ../findunindexeditemsjob.cpp ../nepomukhelpers.cpp ../pluginloader.cpp)
target_link_libraries( akonadinepomukfeeder_indexer 
  ${NEPOMUKFEEDER_SHARED_LIBS}
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  nepomukfeederpluginlib
)

kde4_add_executable(performancetest performancetest.cpp ../nepomukhelpers.cpp ../propertycache.cpp ../pluginloader.cpp)
target_link_libraries( performancetest 
  ${QT_QTTEST_LIBRARY}
  ${NEPOMUKFEEDER_SHARED_LIBS}
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  nepomukfeederpluginlib
)

kde4_add_executable(querytest querytest.cpp ../nepomukhelpers.cpp ../pluginloader.cpp ../findunindexeditemsjob.cpp)
target_link_libraries( querytest 
  ${QT_QTTEST_LIBRARY}
  ${NEPOMUKFEEDER_SHARED_LIBS}
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  nepomukfeederpluginlib
)

kde4_add_executable(cachetest cachetest.cpp ../propertycache.cpp)
target_link_libraries( cachetest 
  ${QT_QTTEST_LIBRARY}
  ${NEPOMUKFEEDER_SHARED_LIBS}
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  ${KDEPIMLIBS_KMIME_LIBS}
  nepomukfeederpluginlib
)