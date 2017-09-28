find_package(PkgConfig)
include(FindPackageHandleStandardArgs)

find_library(CALENDARING_KDECORE NAMES calendaring-kdecore)
find_library(CALENDARING_KCALCORE NAMES calendaring-kcalcore)
find_library(CALENDARING_KMIME NAMES calendaring-kmime)
find_library(CALENDARING_KIMAP NAMES calendaring-kimap)
find_library(CALENDARING_KABC NAMES calendaring-kabc)
find_library(CALENDARING_NOTES NAMES calendaring-akonadi-notes)
find_library(CALENDARING_KCALUTILS NAMES calendaring-kcalutils)
find_library(CALENDARING_KPIMUTILS NAMES calendaring-kpimutils)

find_path(CALENDARING_INCLUDE_DIRS NAMES calendaring/kdatetime.h)

set( Libcalendaring_INCLUDE_DIRS "${CALENDARING_INCLUDE_DIRS}/calendaring" )

set( Libcalendaring_LIBRARIES ${CALENDARING_KDECORE} ${CALENDARING_KCALCORE} ${CALENDARING_KMIME} ${CALENDARING_KIMAP} ${CALENDARING_KABC} ${CALENDARING_NOTES} ${CALENDARING_KCALUTILS} ${CALENDARING_KPIMUTILS})
set(KdepimLibs_VERSION_MAJOR 4)
set(KdepimLibs_VERSION_MINOR 10)
set(KdepimLibs_VERSION_PATCH 0)

find_package_handle_standard_args(Libcalendaring  DEFAULT_MSG
                                  CALENDARING_KDECORE CALENDARING_KCALCORE CALENDARING_KMIME CALENDARING_KIMAP CALENDARING_KABC CALENDARING_NOTES CALENDARING_KCALUTILS CALENDARING_KPIMUTILS CALENDARING_INCLUDE_DIRS)
