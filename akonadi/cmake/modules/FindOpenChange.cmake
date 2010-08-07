# - Try to find the OpenChange MAPI library
# Once done this will define
#
#  LIBMAPI_FOUND - system has OpenChange MAPI library (libmapi)
#  LIBMAPI_INCLUDE_DIRS - the libmapi include directories
#  LIBMAPI_LIBRARIES - Required libmapi link libraries
#  LIBMAPI_DEFINITIONS - Compiler switches for libmapi
#
# Copyright (C) 2007 Brad Hards (bradh@frogmouth.net)
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the COPYING-CMAKE-SCRIPTS file in kdelibs/cmake/modules/

if (LIBMAPI_INCLUDE_DIRS AND LIBMAPI_LIBRARIES)

  # in cache already
  SET(LIBMAPI_FOUND TRUE)

else (LIBMAPI_INCLUDE_DIRS AND LIBMAPI_LIBRARIES)
  if(NOT WIN32)
    find_package(PkgConfig)
    pkg_check_modules(libmapi QUIET libmapi)
  endif(NOT WIN32)

  set(LIBMAPI_DEFINITIONS ${libmapi_CFLAGS})
  set(LIBMAPI_INCLUDE_DIRS ${libmapi_INCLUDE_DIRS})
  find_library(LIBMAPIPP_LIBRARY NAMES mapipp PATHS ${libmapi_LIBRARY_DIRS})
  find_library(LIBMAPI_LIBRARY NAMES mapi PATHS ${libmapi_LIBRARY_DIRS})
  find_library(LIBDCERPC_LIBRARY NAMES dcerpc PATHS ${libmapi_LIBRARY_DIRS})

  if (LIBMAPI_INCLUDE_DIRS AND LIBMAPIPP_LIBRARY AND LIBMAPI_LIBRARY AND LIBDCERPC_LIBRARY)
     set(LIBMAPI_FOUND TRUE)
  endif (LIBMAPI_INCLUDE_DIRS AND LIBMAPIPP_LIBRARY AND LIBMAPI_LIBRARY AND LIBDCERPC_LIBRARY)
  
  if (LIBMAPI_FOUND)
    if (NOT OpenChange_FIND_QUIETLY)
      message(STATUS "Found OpenChange MAPI C++ bindings: ${LIBMAPIPP_LIBRARY}")
      message(STATUS "Found OpenChange MAPI library: ${LIBMAPI_LIBRARY}")
      message(STATUS "Found Samba DCERPC library: ${LIBDCERPC_LIBRARY}")
    endif (NOT OpenChange_FIND_QUIETLY)
  else (LIBMAPI_FOUND)
    if (OpenChange_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find OpenChange MAPI libraries")
    endif (OpenChange_FIND_REQUIRED)
  endif (LIBMAPI_FOUND)
  
  MARK_AS_ADVANCED(LIBMAPI_INCLUDE_DIRS LIBMAPIPP_LIBRARY LIBMAPI_LIBRARY LIBDCEPRC_LIBRARY LIBMAPI_DEFINITIONS)
  
endif (LIBMAPI_INCLUDE_DIRS AND LIBMAPI_LIBRARIES)
