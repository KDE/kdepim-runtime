#Generate PHP wrapper
include_directories(../)
include(SWIGUtils)
generatePHPBindings(kolabicalendar ../icalendar.i)
