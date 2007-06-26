macro_optional_find_package( Plasma )
macro_log_feature(PLASMA_FOUND "plasma" "Plasma libraries and header files" "http://plasma.kde.org/" FALSE "" "Needed for Akonadi Plasma stuff")

if (PLASMA_FOUND)
  add_subdirectory(engine)
  add_subdirectory(plasmobiff)
endif (PLASMA_FOUND)

