%{
    /* This macro ensures that return vectors remain a vector also in python and are not converted to tuples */
    #define SWIG_PYTHON_EXTRA_NATIVE_CONTAINERS
    
    #include "../calendaring/calendaring.h"
    #include "../calendaring/event.h"
%}

%include "std_string.i"
%include "std_vector.i"

%import(module="kolabformat") <kolabevent.h>
%import "../shared.i"

%rename(EventCal) Kolab::Calendaring::Event;
%rename(KolabCalendar) Kolab::Calendaring::Calendar;

%include "../calendaring/calendaring.h"
%include "../calendaring/event.h"
