#
# - Try to find the sasl2 directory library
# Once done this will define
#
#  Sasl2_FOUND - system has SASL2
#  Sasl2_INCLUDE_DIRS - the SASL2 include directory
#  Sasl2_LIBRARIES - The libraries needed to use SASL2

# Copyright (c) 2006, 2007 Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Note: libsasl2.pc doesn't export the include dir.
find_package(PkgConfig QUIET)
pkg_check_modules(PC_Sasl2 libsasl2)

find_path(Sasl2_INCLUDE_DIRS sasl/sasl.h
)

# libsasl2 add for windows, because the windows package of cyrus-sasl2
# contains a libsasl2 also for msvc which is not standard conform
find_library(Sasl2_LIBRARIES
  NAMES sasl2 libsasl2
  HINTS ${PC_Sasl2_LIBRARY_DIRS}
)

set(Sasl2_VERSION ${PC_Sasl2_VERSION})

if(NOT Sasl2_VERSION)
  if(EXISTS ${Sasl2_INCLUDE_DIRS}/sasl/sasl.h)
    file(READ ${Sasl2_INCLUDE_DIRS}/sasl/sasl.h SASL2_H_CONTENT)
    string(REGEX MATCH "#define SASL_VERSION_MAJOR[ ]+[0-9]+" SASL2_VERSION_MAJOR_MATCH ${SASL2_H_CONTENT})
    string(REGEX MATCH "#define SASL_VERSION_MINOR[ ]+[0-9]+" SASL2_VERSION_MINOR_MATCH ${SASL2_H_CONTENT})
    string(REGEX MATCH "#define SASL_VERSION_STEP[ ]+[0-9]+" SASL2_VERSION_STEP_MATCH ${SASL2_H_CONTENT})

    string(REGEX REPLACE ".*_MAJOR[ ]+(.*)" "\\1" SASL2_VERSION_MAJOR ${SASL2_VERSION_MAJOR_MATCH})
    string(REGEX REPLACE ".*_MINOR[ ]+(.*)" "\\1" SASL2_VERSION_MINOR ${SASL2_VERSION_MINOR_MATCH})
    string(REGEX REPLACE ".*_STEP[ ]+(.*)" "\\1"  SASL2_VERSION_STEP  ${SASL2_VERSION_STEP_MATCH})

    set(Sasl2_VERSION "${SASL2_VERSION_MAJOR}.${SASL2_VERSION_MINOR}.${SASL2_VERSION_STEP}")
  endif()
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Sasl2
    FOUND_VAR Sasl2_FOUND
    REQUIRED_VARS Sasl2_LIBRARIES Sasl2_INCLUDE_DIRS
    VERSION_VAR Sasl2_VERSION
)

mark_as_advanced(Sasl2_LIBRARIES Sasl2_INCLUDE_DIRS Sasl2_VERSION)
