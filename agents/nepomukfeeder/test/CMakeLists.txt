include_directories(
        ../
)

kde4_add_executable(akonadinepomukfeeder_indexer akonadinepomukfeeder_indexer.cpp ../feederqueue.cpp ../nepomukhelpers.cpp ../pluginloader.cpp)
target_link_libraries( akonadinepomukfeeder_indexer 
  ${NEPOMUKFEEDER_SHARED_LIBS}
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  nepomukfeederpluginlib
)