
%{
    /* This macro ensures that return vectors remain a vector also in python and are not converted to tuples */
    #define SWIG_PYTHON_EXTRA_NATIVE_CONTAINERS
    #include <kolabevent.h>
%}
%include "std_vector.i"
%import(module="kolabformat") <kolabevent.h>
namespace std {
/* vectorevent moved to libkolabxml, vectorevent2 breaks the pythonbindings without vectorevent in here (compile error) */
/*    %template(vectorevent) vector<Kolab::Event>; */
/*    %template(vectorevent2) vector< vector<Kolab::Event> >; */
};
