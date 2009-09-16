# a CMake script. envoke with the -P option

# must define
# targetdir
# ontofile1, ontofile2, ontofile3, ontofile4
# _rcgen

file(TO_NATIVE_PATH ${_rcgen} _rcgen)

execute_process(
  COMMAND ${_rcgen} --listheaders --prefix ${targetdir}/ --ontologies ${ontofile1} ${ontofile2} ${ontofile3} ${ontofile4}
  OUTPUT_VARIABLE _out_headers
  RESULT_VARIABLE _rcgen_result
  ERROR_QUIET
  )
file(WRITE ${targetdir}/out_headers "${_out_headers}")

# If the first call succeeds it is very very likely that the rest will, too
if(${_rcgen_result} EQUAL 0)

  execute_process(
    COMMAND ${_rcgen} --listsources --prefix ${targetdir}/ --ontologies ${ontofile1} ${ontofile2} ${ontofile3} ${ontofile4}
    OUTPUT_VARIABLE _out_sources
    ERROR_QUIET
    )
  file(WRITE ${targetdir}/out_sources "${_out_sources}")

  execute_process(
    COMMAND ${_rcgen} --listincludes --ontologies ${ontofile1} ${ontofile2} ${ontofile3} ${ontofile4}
    OUTPUT_VARIABLE _out_includes
    ERROR_QUIET
    )
  file(WRITE ${targetdir}/out_includes "${_out_includes}")

  execute_process(
    COMMAND ${_rcgen} --writeall --templates "DUMMY" --target ${targetdir}/ --ontologies ${ontofile1} ${ontofile2} ${ontofile3} ${ontofile4}
    ERROR_QUIET
    )

else(${_rcgen_result} EQUAL 0)

  message(FATAL_ERROR "Failed to generate Nepomuk resource classes. Do you have a recent Soprano version?")

endif(${_rcgen_result} EQUAL 0)

