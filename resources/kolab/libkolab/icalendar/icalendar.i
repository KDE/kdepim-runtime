%{
    /* This macro ensures that return vectors remain a vector also in python and are not converted to tuples */
    #define SWIG_PYTHON_EXTRA_NATIVE_CONTAINERS

    #include "icalendar.h"
%}

%include "std_string.i"

%import(module="kolabformat") <kolabevent.h>
%import "../shared.i"

%include "icalendar.h"