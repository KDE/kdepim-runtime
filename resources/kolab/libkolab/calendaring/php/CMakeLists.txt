#Generate PHP wrapper
include_directories(../)
include(SWIGUtils)
generatePHPBindings(kolabcalendaring ../calendaring.i)
