#Generate PHP wrapper
include_directories(../)
include(SWIGUtils)
generatePHPBindings(kolabobject ../kolabobject.i)
