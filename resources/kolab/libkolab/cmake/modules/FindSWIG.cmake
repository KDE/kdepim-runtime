find_program(SWIG swig /usr/bin/)

#abort if any of the requireds are missing
find_package_handle_standard_args(SWIG DEFAULT_MSG SWIG)
