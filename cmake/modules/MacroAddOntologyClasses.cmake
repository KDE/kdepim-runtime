#
# Use the Nepomuk resource class generator to generate convinient Resource subclasses
# from ontologies.
#
# Usage:
#   NEPOMUK_ADD_ONTOLOGY_CLASSES(<sources-var>
#         [FAST]
#         [ONTOLOGIES] <onto-file1> [<onto-file2> ...]
#         [TEMPLATES <template1> [<template2> ...]]
#       )
#
# Author: Sebastian Trueg <trueg@kde.org
#
macro(NEPOMUK_ADD_ONTOLOGY_CLASSES _sources)
  # extract ontologies and templates from arguments
  set(_current_arg_type "onto")
  foreach(_arg ${ARGN})
    if(${_arg} STREQUAL "ONTOLOGIES")
      set(_current_arg_type "onto")
    elseif(${_arg} STREQUAL "FAST")
      set(_fastmode "--fast")
    elseif(${_arg} STREQUAL "TEMPLATES")
      set(_current_arg_type "templ")
    else(${_arg} STREQUAL "ONTOLOGIES")
      if(${_current_arg_type} STREQUAL "onto")
        list(APPEND _ontologies ${_arg})
        get_filename_component(_filename ${_arg} NAME)
        list(APPEND _ontofilenames ${_filename})
      else(${_current_arg_type} STREQUAL "onto")
        list(APPEND _templates ${_arg})
      endif(${_current_arg_type} STREQUAL "onto")
    endif(${_arg} STREQUAL "ONTOLOGIES")
  endforeach(_arg)

  # find our helper program (first in the install dir, then everywhere)
  find_program(RCGEN nepomuk-rcgen PATHS ${KDE4_BIN_INSTALL_DIR} ${BIN_INSTALL_DIR} NO_DEFAULT_PATH)
  find_program(RCGEN nepomuk-rcgen)

  if(NOT RCGEN)
    message(SEND_ERROR "Failed to find the Nepomuk source generator" )
  else(NOT RCGEN)
    FILE(TO_NATIVE_PATH ${RCGEN} RCGEN)

    # we generate the files in the current binary dir
    set(_targetdir ${CMAKE_CURRENT_BINARY_DIR})

    # generate the list of source and header files
    execute_process(
      COMMAND ${RCGEN} ${_fastmode} --listheaders --prefix ${_targetdir}/ --ontologies ${_ontologies}
      OUTPUT_VARIABLE _out_headers
      RESULT_VARIABLE rcgen_result
      )
    execute_process(
      COMMAND ${RCGEN} ${_fastmode} --listsources --prefix ${_targetdir}/ --ontologies ${_ontologies}
      OUTPUT_VARIABLE _out_sources
      )

    # If the first call succeeds it is very very likely that the rest will, too
    if(NOT ${rcgen_result} EQUAL 0)
      message(SEND_ERROR "Failed to generate Nepomuk resource classes list.")
    endif(NOT ${rcgen_result} EQUAL 0)

    add_custom_command(OUTPUT ${_out_headers} ${_out_sources}
      COMMAND ${RCGEN} ${_fastmode} --writeall --templates ${_templates} --target ${_targetdir}/ --ontologies ${_ontologies}
      DEPENDS ${_ontologies}
      COMMENT "Generating ontology source files from ${_ontofilenames}"
      )

    # make sure the includes are found
    include_directories(${_targetdir})

    # finally append the source files to the source list
    list(APPEND ${_sources} ${_out_sources})
  endif(NOT RCGEN)
endmacro(NEPOMUK_ADD_ONTOLOGY_CLASSES)
