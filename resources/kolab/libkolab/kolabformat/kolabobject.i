%{
    /* This macro ensures that return vectors remain a vector also in python and are not converted to tuples */
    #define SWIG_PYTHON_EXTRA_NATIVE_CONTAINERS
    
    #include "../kolabformat/xmlobject.h"
    #include "../kolabformat/kolabdefinitions.h"
    #include "../kolabformat/mimeobject.h"
%}

%include "std_string.i"
%include "std_vector.i"

%import(module="kolabformat") <kolabevent.h>
%import "../shared.i"

%include "../kolabformat/xmlobject.h"
%include "../kolabformat/kolabdefinitions.h"
%include "../kolabformat/mimeobject.h"
