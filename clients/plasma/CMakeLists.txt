# Once the Akonadi libs move into the kdepimlibs module,
# please move this Plasmoid into the kdeaddons module.
# We don't want to have kdepim dependent on kdebase.

macro_optional_find_package( Plasma )
macro_log_feature(PLASMA_FOUND "plasma" "Plasma libraries and header files" "http://plasma.kde.org/" FALSE "" "Needed for Akonadi Plasma stuff")

if (PLASMA_FOUND)
  add_subdirectory(engine)
  add_subdirectory(plasmobiff)
endif (PLASMA_FOUND)

